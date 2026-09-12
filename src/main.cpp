#include <Geode/Geode.hpp>

#include <windows.h>
#include <algorithm>
#include <vector>
#include <numeric>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/binding/ButtonSprite.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/FLAlertLayer.hpp>

using namespace geode::prelude;

// NtSetTimerResolution uses 100-nanosecond units.
using NtSetTimerResolution_t =
    LONG(NTAPI*)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);

// NtQueryTimerResolution returns:
// - MaximumResolution
// - MinimumResolution
// - CurrentResolution
//
// All values are in 100-nanosecond units.
using NtQueryTimerResolution_t =
    LONG(NTAPI*)(PULONG MaximumResolution, PULONG MinimumResolution, PULONG CurrentResolution);

static NtSetTimerResolution_t g_NtSetTimerResolution = nullptr;
static NtQueryTimerResolution_t g_NtQueryTimerResolution = nullptr;

static ULONG g_currentRequestedResolution = 0;

static bool initializeTimerResolutionAPI() {
    auto ntdll = GetModuleHandleW(L"ntdll.dll");

    if (!ntdll)
        return false;

    g_NtSetTimerResolution = reinterpret_cast<NtSetTimerResolution_t>(
        GetProcAddress(ntdll, "NtSetTimerResolution")
    );

    g_NtQueryTimerResolution = reinterpret_cast<NtQueryTimerResolution_t>(
        GetProcAddress(ntdll, "NtQueryTimerResolution")
    );

    return g_NtSetTimerResolution != nullptr &&
           g_NtQueryTimerResolution != nullptr;
}

static bool queryTimerResolution(
    ULONG& maximumResolution,
    ULONG& minimumResolution,
    ULONG& currentResolution
) {
    if (!g_NtQueryTimerResolution && !initializeTimerResolutionAPI()) {
        log::error("Failed to find NtQueryTimerResolution");
        return false;
    }

    LONG status = g_NtQueryTimerResolution(
        &maximumResolution,
        &minimumResolution,
        &currentResolution
    );

    if (status < 0) {
        log::error(
            "NtQueryTimerResolution failed: NTSTATUS 0x{:08X}",
            static_cast<unsigned long>(status)
        );
        return false;
    }

    return true;
}

static bool setTimerResolution(double milliseconds) {
    if (!g_NtSetTimerResolution && !initializeTimerResolutionAPI()) {
        log::error("Failed to find NtSetTimerResolution");
        return false;
    }

    auto requested =
        static_cast<ULONG>(milliseconds * 10000.0 + 0.5);

    ULONG currentResolution = 0;

    // Release our previous request first.
    if (g_currentRequestedResolution != 0) {
        ULONG ignored = 0;

        g_NtSetTimerResolution(
            g_currentRequestedResolution,
            FALSE,
            &ignored
        );

        g_currentRequestedResolution = 0;
    }

    LONG status = g_NtSetTimerResolution(
        requested,
        TRUE,
        &currentResolution
    );

    if (status < 0) {
        log::error(
            "NtSetTimerResolution failed: NTSTATUS 0x{:08X}",
            static_cast<unsigned long>(status)
        );
        return false;
    }

    g_currentRequestedResolution = requested;

    log::info(
        "Requested timer resolution: {:.4f} ms | Current resolution: {:.4f} ms",
        milliseconds,
        static_cast<double>(currentResolution) / 10000.0
    );

    return true;
}

static double getCurrentRequestedResolutionMs() {
    if (g_currentRequestedResolution == 0)
        return 0.0;

    return static_cast<double>(g_currentRequestedResolution) / 10000.0;
}

static void runSleepTest() {
    constexpr int testCount = 100;

    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;

    if (!QueryPerformanceFrequency(&frequency)) {
        FLAlertLayer::create(
            "Timer Test",
            "QueryPerformanceFrequency failed.",
            "OK"
        )->show();

        return;
    }

    std::vector<double> measurements;
    measurements.reserve(testCount);

    for (int i = 0; i < testCount; ++i) {
        QueryPerformanceCounter(&start);

        Sleep(1);

        QueryPerformanceCounter(&end);

        auto elapsedMs =
            static_cast<double>(end.QuadPart - start.QuadPart) *
            1000.0 /
            static_cast<double>(frequency.QuadPart);

        measurements.push_back(elapsedMs);
    }

    auto totalMs =
        std::accumulate(
            measurements.begin(),
            measurements.end(),
            0.0
        );

    auto averageMs =
        totalMs / static_cast<double>(testCount);

    auto minimumMs =
        *std::min_element(
            measurements.begin(),
            measurements.end()
        );

    auto maximumMs =
        *std::max_element(
            measurements.begin(),
            measurements.end()
        );

    auto requestedMs = getCurrentRequestedResolutionMs();

    ULONG maximumResolution = 0;
    ULONG minimumResolution = 0;
    ULONG currentResolution = 0;

    queryTimerResolution(
        maximumResolution,
        minimumResolution,
        currentResolution
    );

    auto actualMs =
        static_cast<double>(currentResolution) / 10000.0;

    auto message = fmt::format(
        "<cy>Make sure you clicked Apply after changing "
        "the resolution before running this test.</c>\n\n"
        "Requested: <cy>{:.4f} ms</c>\n"
        "Actual: <cy>{:.4f} ms</c>\n\n"
        "Average: <cy>{:.3f} ms</c>\n"
        "Minimum: <cy>{:.3f} ms</c>\n"
        "Maximum: <cy>{:.3f} ms</c>",
        requestedMs,
        actualMs,
        averageMs,
        minimumMs,
        maximumMs
    );

    FLAlertLayer::create(
        "Sleep Test",
        message,
        "OK"
    )->show();
}

class TimerresTestButtonSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(
        std::string const& key,
        std::string const& modID,
        matjson::Value const& json
    ) {
        auto res = std::make_shared<TimerresTestButtonSettingV3>();
        auto root = checkJson(json, "TimerresTestButtonSettingV3");

        // Initialize the base SettingV3 properties.
        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);

        root.checkUnknownKeys();

        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override {
        return true;
    }

    bool save(matjson::Value& json) const override {
        return true;
    }

    bool isDefaultValue() const override {
        return true;
    }

    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};


class TimerresTestButtonNodeV3 : public SettingNodeV3 {
protected:
    ButtonSprite* m_buttonSprite = nullptr;
    CCMenuItemSpriteExtra* m_button = nullptr;

    bool init(
        std::shared_ptr<TimerresTestButtonSettingV3> setting,
        float width
    ) {
        if (!SettingNodeV3::init(setting, width))
            return false;

        m_buttonSprite = ButtonSprite::create(
            "Test Timer",
            "goldFont.fnt",
            "GJ_button_01.png",
            .8f
        );

        m_buttonSprite->setScale(.5f);

        m_button = CCMenuItemSpriteExtra::create(
            m_buttonSprite,
            this,
            menu_selector(TimerresTestButtonNodeV3::onButton)
        );

        this->getButtonMenu()->addChildAtPosition(
            m_button,
            Anchor::Center
        );

        this->getButtonMenu()->setContentWidth(70);
        this->getButtonMenu()->updateLayout();

        this->updateState(nullptr);

        return true;
    }

    void updateState(CCNode* invoker) override {
        SettingNodeV3::updateState(invoker);

        auto shouldEnable = this->getSetting()->shouldEnable();

        m_button->setEnabled(shouldEnable);

        m_buttonSprite->setCascadeColorEnabled(true);
        m_buttonSprite->setCascadeOpacityEnabled(true);

        m_buttonSprite->setOpacity(
            shouldEnable ? 255 : 155
        );

        m_buttonSprite->setColor(
            shouldEnable ? ccWHITE : ccGRAY
        );
    }

    void onButton(CCObject*) {
    runSleepTest();
    }

    void onCommit() override {}

    void onResetToDefault() override {}

public:
    static TimerresTestButtonNodeV3* create(
        std::shared_ptr<TimerresTestButtonSettingV3> setting,
        float width
    ) {
        auto ret = new TimerresTestButtonNodeV3();

        if (ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }

        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override {
        return false;
    }

    bool hasNonDefaultValue() const override {
        return false;
    }

    std::shared_ptr<TimerresTestButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<TimerresTestButtonSettingV3>(
            SettingNodeV3::getSetting()
        );
    }
};


SettingNodeV3* TimerresTestButtonSettingV3::createNode(float width) {
    return TimerresTestButtonNodeV3::create(
        std::static_pointer_cast<TimerresTestButtonSettingV3>(
            shared_from_this()
        ),
        width
    );
}

$on_mod(Loaded) {
    auto resolution =
        Mod::get()->getSettingValue<double>("resolution");

    setTimerResolution(resolution);

    listenForSettingChanges<double>(
        "resolution",
        [](double value) {
            setTimerResolution(value);
        }
    );

    (void)Mod::get()->registerCustomSettingType(
        "timerres-test-button",
        &TimerresTestButtonSettingV3::parse
    );
}