#include<Geode/Geode.hpp>

#include<windows.h>

#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0) // implementing the whole ntdef.h header just for this is kinda unnecessary

// No more C type conversion, also mod shrunk significantly... what
// why easy when you can do it complicated ig... now its easy fortunately

using namespace geode::prelude;

static bool isRequest = false;

extern "C" NTSYSAPI NTSTATUS NTAPI NtSetTimerResolution(ULONG RequestedRes, BOOLEAN DoWeSetRes, PULONG CurrentRes); // thank you valleyofdoom

void setTR(double msreqRes) {
	ULONG currentRes;
	ULONG reqRes;

	reqRes = static_cast<ULONG>(msreqRes * 10000);
	
	if(isRequest == true){
		if(NT_SUCCESS(NtSetTimerResolution(0, FALSE, &currentRes)) == false){
			log::warn("Removing the old Timer Resolution failed. Please try again");
			return;
		}
		isRequest = false;
	}
	if(NT_SUCCESS(NtSetTimerResolution(reqRes, TRUE, &currentRes)) == false){
		log::warn("Changing the Timer Resolution failed! Please try again");
		return;
	}
	log::debug("Current Timer Resolution: {}", currentRes);
	
	isRequest = true; // Found out C++17 removed bool++... I hate it for that
}

$on_mod(Loaded) { 
	setTR(Mod::get()->getSettingValue<double>("resolution"));

	listenForSettingChanges<double>("resolution", [](double setting){
		setTR(setting);
	}, Mod::get());
}

$on_game(Exiting) {
	ULONG currentRes;
	if(isRequest == true){
		if(NT_SUCCESS(NtSetTimerResolution(0, FALSE, &currentRes)) == false){
			log::warn("Removing the old Timer Resolution failed. Retrying...");
			if(NT_SUCCESS(NtSetTimerResolution(0, FALSE, &currentRes)) == false){ // tbh this should never happen, if it does Windows will go back to default when GD closes
				log::error("Removing the old Timer Resolution failed again.");
			}
		}
	}
}