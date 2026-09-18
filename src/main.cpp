#include<Geode/Geode.hpp>

#include<windows.h>

// Please excuse my severe lack of knowledge of C++ (mostly just me using C type conversion), I come from the lands of C

using namespace geode::prelude;
 
using NtSetTRtype = LONG(NTAPI*)(ULONG RequestedRes, BOOLEAN DoWeSetRes, PULONG CurrentRes);
using NtQueryTRtype = LONG(NTAPI*)(PULONG MaxRes, PULONG MinRes, PULONG CurrentRes);

static NtSetTRtype NtSetTR = nullptr;
static NtQueryTRtype NtQueryTR = nullptr;

static bool isRequest = false; // I gave up, I used global here, mainly for the cleanup on unload
static ULONG reqRes;

bool init() {

	auto ntdll = GetModuleHandleW(L"ntdll.dll");

	if (ntdll == nullptr){
		log::error("Failed to find Windows API! TimerRes will not work. (if this fails there are bigger issues at hand, check your system)");
		log::debug("GetModuleHandle has failed. yikes (are you even running Windows?)");
		return false;
	}

	NtSetTR = (NtSetTRtype)GetProcAddress(ntdll, "NtSetTimerResolution");
	if(NtSetTR == nullptr){
		log::error("Windows API has failed! TimerRes will not work.");
		log::debug("GetProcAddress (setTR) has failed. yikes (are you even running Windows?)");
		return false;
	}

	NtQueryTR = (NtQueryTRtype)GetProcAddress(ntdll, "NtQueryTimerResolution");
	if(NtQueryTR == nullptr){
		log::error("Windows API has failed! TimerRes will not work.");
		log::debug("GetProcAddress (queryTR) has failed. yikes (are you even running Windows?)");
		return false;
	}

	return true;
}

bool setTR(float msreqRes) {
	ULONG currentRes;
	NTSTATUS setstatus;
	
	if (isRequest == true){
		setstatus = NtSetTR(reqRes, false, &currentRes);
		if(setstatus < 0){
			log::info("Removing the old Timer Resolution failed. As you can see this is [info] as this doesn't impact functionality at all lolololo");
		}
	}
	reqRes = (ULONG)(msreqRes * 10000);
	setstatus = NtSetTR(reqRes, true, &currentRes);
	if(setstatus < 0){
		log::warn("Changing the Timer Resolution failed! Please try again");
		return false;
	}
	log::debug("Current Res: {}", currentRes);
	// old code: 	isRequest = !isRequest; // my poor bool++, I hate you C++17
	isRequest = true;

	return true;
}

bool queryTR(PULONG actualRes) {
	ULONG maxRes;
	ULONG minRes;
	NTSTATUS querystatus = NtQueryTR(&maxRes, &minRes, actualRes);

	if(querystatus < 0){
		log::warn("Querying the Timer Resolution failed! Please try again");
		return false;
	}
	log::debug("Actual Res: {}", (float)*actualRes / 10000); // be happy I didn't use implicit conversion (*actualRes / 10000.0)

	return true;
}

$on_mod(Loaded) {
	
	if (init() == false){
		return;
	}
	
	float resolution = Mod::get()->getSettingValue<float>("resolution");
	if (setTR(resolution) == false){
		return;
	}
	
	ULONG actualRes;
	if (queryTR(&actualRes) == false){
		return;
	}

	listenForSettingChanges<float>("resolution", [](float setting){
		if (setTR(setting) == false){
			return;
		}
	});
}

$on_mod(Unloaded) {
	if(isRequest == false){
		return;
	}
	
	ULONG currentRes;
	NTSTATUS setstatus = NtSetTR(reqRes, false, &currentRes);
	if(setstatus < 0){
		log::warn("Removing the Timer Resolution failed! Let's see what happens now");
		return;
	}
	log::debug("Timer Resolution successfully removed :) your CPU can go back to rest");
}