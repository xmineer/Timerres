#include <Geode/Geode.hpp>

#include <windows.h>

// Sorry if these comments seem kinda unprofessional, I just like having fun making mods :]

// No more C type conversion, also mod shrunk significantly... what
// why easy when you can do it complicated ig... now its easy fortunately

// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FTime%2FNtSetTimerResolution.html		regarding the Timer Resolution function

using namespace geode::prelude;

extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG RequestedRes, BOOLEAN DoWeSetRes, PULONG CurrentRes); // thank you valleyofdoom, credit to them for this line

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0) // implementing the whole ntdef.h header just for this is kinda unnecessary

class TimerResolutionManager {
private:
	bool m_isRequest = false; // whole ass class for this

public:
	bool clearTR() {
		ULONG currentRes;
		if (m_isRequest) {
			if (!NT_SUCCESS(NtSetTimerResolution(0, FALSE, &currentRes))) {
				return false;
			}
			m_isRequest = false;
		}
		return true;
	}

	void setTR(double msReqRes) {
		ULONG currentRes;
		ULONG reqRes = static_cast<ULONG>(msReqRes * 10000);
		if (!clearTR()) {
			log::warn("Removing the old Timer Resolution failed. Please try again");
			FLAlertLayer::create(
				"Timerres",
				"Removing the old Timer Resolution failed. Please try again",
				"Ok"
			)->show();
			return;
		}
		if (!NT_SUCCESS(NtSetTimerResolution(reqRes, TRUE, &currentRes))) {
			log::warn("Changing the Timer Resolution failed! Please try again");
			FLAlertLayer::create(
				"Timerres",
				"Changing the Timer Resolution failed! Please try again.",
				"Ok"
			)->show();
			return;
		}
		m_isRequest = true;
		if (reqRes != currentRes) {
			log::warn("Windows has rejected your requested Resolution.");
			FLAlertLayer::create(
				"Timerres",
				fmt::format("Windows could not apply your <cs>{:.4f} ms</c> resolution.\nUsing <cs>{:.4f} ms</c> instead.\nPlease adjust your value accordingly.", msReqRes, static_cast<double>(currentRes) / 10000),
				"Ok"
			)->show();
		}
		log::debug("Current Timer Resolution: {}", currentRes);
	}
};

#undef NT_SUCCESS // mitigate random problems by not leaking a macro I took from Windows

TimerResolutionManager& getTRM() {
    static TimerResolutionManager s_TRManager;
    return s_TRManager;
}

$on_mod(Loaded) { 
	getTRM().setTR(Mod::get()->getSettingValue<double>("resolution"));

	listenForSettingChanges<double>("resolution", [](double setting) {
		getTRM().setTR(setting);
	}, Mod::get());
}


$on_game(Exiting) {
	getTRM().clearTR(); // if it fails Windows will go back to default when GD closes
}