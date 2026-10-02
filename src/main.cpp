#include <Geode/Geode.hpp>

#include <windows.h>

// Sorry if these comments seem kinda unprofessional, I just like having fun making mods :]

// No more C type conversion, also mod shrunk significantly... what
// why easy when you can do it complicated ig... now its easy fortunately

// http://undocumented.ntinternals.net/index.html?page=UserMode%2FUndocumented%20Functions%2FTime%2FNtSetTimerResolution.html
// regarding the Timer Resolution function

using namespace geode::prelude;

// ntdef.h (where this macro is from) is for kernel-level apps
// and should not be implemented here so I just took the macro
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0) 

// thank you valleyofdoom, credit to them for this
extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(
	ULONG RequestedRes,
	BOOLEAN DoWeSetRes,
	PULONG CurrentRes
); // hate how multiple lines look but necessary to not break 80 characters rule

class TimerResolutionManager final {
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
		ULONG currentRes = 0;
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
			log::warn("Windows could not apply your requested Resolution.");
			FLAlertLayer::create(
				"Timerres",
				fmt::format("Windows could not apply your <cs>{:.4f} ms</c> resolution.\n"
					"Using <cs>{:.4f} ms</c> instead.\n"
					"Please adjust your value accordingly.",
					msReqRes, static_cast<double>(currentRes) / 10000),
				"Ok"
			)->show();
		}
		log::debug("Current Timer Resolution: {}", currentRes);
	}
};

#undef NT_SUCCESS // mitigate random problems by not leaking a kernel-mode macro

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
	if (!getTRM().clearTR()) { // if it fails Windows will remove the request itself when GD closes
		log::warn("Removing the Timer Resolution failed.");
	} 
}