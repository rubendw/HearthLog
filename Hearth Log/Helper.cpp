#include <wx/stdpaths.h>
#include <wx/log.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>

#include "Helper.h"

const std::string &Helper::AppVersion()
{
	static const std::string version = "v0.2.1";
	return version;
}

wxFileName Helper::GetUserDataDir()
{
	return wxFileName(wxStandardPaths::Get().GetUserDataDir(), "");
}

#ifdef WIN32
#include <windows.h>
#pragma comment(lib, "version.lib")
std::uint64_t Helper::GetHearthstoneVersion()
{
	auto dir = ReadConfig("HearthstoneDir", wxString("C:\\Program Files (x86)\\Hearthstone"));
	wxFileName file(dir, "Hearthstone.exe");
	auto path = file.GetFullPath();
	if (!file.Exists()) {
		wxLogError("couldn't find %s", path);
		return 0;
	}
	
	DWORD verHandle = NULL;
	DWORD verSize   = GetFileVersionInfoSize(path.t_str(), &verHandle);
	if (!verSize) {
		wxLogError("GetFileVersionInfoSize(%s): %d", path, GetLastError());
		return 0;
	}

	LPSTR  verData  = new char[verSize];
	UINT   size     = 0;
	LPBYTE lpBuffer = NULL;
	if (!GetFileVersionInfo(path.t_str(), verHandle, verSize, verData)) {
		wxLogError("GetFileVersionInfo(%s): %d", path, GetLastError());
	} else if (!VerQueryValue(verData, L"\\", (LPVOID*)&lpBuffer, &size)) {
		wxLogError("VerQueryValue(%s): %d", path, GetLastError());
	} else if (!size) {
		wxLogError("no version info", path, GetLastError());
	} else {
		VS_FIXEDFILEINFO *verInfo = (VS_FIXEDFILEINFO *)lpBuffer;
		if (verInfo->dwSignature != 0xfeef04bd) {
			wxLogError("bad version signature (%s)", path);
		} else {
			auto version = (uint64_t)verInfo->dwFileVersionMS << 32 | verInfo->dwFileVersionLS;
			wxLogVerbose("%s v%u.%u.%u.%u", path, 
				(uint16_t)(version >> 48),
				(uint16_t)(version >> 32),
				(uint16_t)(version >> 16),
				(uint16_t)(version));
			return version;
		}
	}
	delete[] verData;

	return 0;
}

bool Helper::FindHearthstone() {
	while (!GetHearthstoneVersion()) {
		auto dir = wxDirSelector(_("Hearth Log: Please locate your Hearthstone directory (Usually C:\\Program Files (x86)\\Hearthstone)"));
		if (dir.empty()) {
			return false;
		}
		Helper::WriteConfig("HearthstoneDir", dir);
	}
	return true;
}
#endif

#ifdef MAC_OS_X_VERSION_MIN_REQUIRED
#include <CoreFoundation/CoreFoundation.h>
#include <CoreFoundation/CFURL.h>
std::uint64_t Helper::GetHearthstoneVersion()
{
 
    const char* path = "/Applications/Hearthstone/Hearthstone.app";
    CFStringRef appPath = CFStringCreateWithCString(NULL, path, kCFStringEncodingUTF8);
    
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, appPath, kCFURLPOSIXPathStyle, false);
    CFBundleRef app = CFBundleCreate(NULL, url);
    CFStringRef dictValue = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(app, CFSTR("BlizzardFileVersion"));
    
    if(dictValue == NULL){
        wxLogError("cannot find Blizzard File Version");
        return 0;
    }
    
    SInt32 first, second, third, fourth;
    CFArrayRef arr = CFStringCreateArrayBySeparatingStrings(NULL, dictValue, CFSTR("."));
    if(CFArrayGetCount(arr)==CFIndex(4)) {
        first = CFStringGetIntValue((CFStringRef)CFArrayGetValueAtIndex(arr, CFIndex(0)));
        second = CFStringGetIntValue((CFStringRef)CFArrayGetValueAtIndex(arr, CFIndex(1)));
        third = CFStringGetIntValue((CFStringRef)CFArrayGetValueAtIndex(arr, CFIndex(2)));
        fourth = CFStringGetIntValue((CFStringRef)CFArrayGetValueAtIndex(arr, CFIndex(3)));
    } else {
        wxLogError("unexpected version value or format");
        return 0;
    }
    
    uint64_t version = first;
    version <<= 16;
    version += second;
    version <<= 16;
    version += third;
    version <<= 16;
    version += fourth;
    
    // same exact log statement as the WIN32 code as a way to validate the number and format
    wxLogVerbose("%s v%u.%u.%u.%u", path,
        (uint16_t)(version >> 48),
        (uint16_t)(version >> 32),
        (uint16_t)(version >> 16),
        (uint16_t)(version));
    
    // memory management
    CFRelease(appPath);
    CFRelease(url);
    CFRelease(app);
    CFRelease(arr);
    
    return version;
}
#endif
