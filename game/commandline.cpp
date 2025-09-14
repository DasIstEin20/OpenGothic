#include "commandline.h"

#include <Tempest/Log>
#include <Tempest/TextCodec>
#include <cstring>
#include <cassert>
#include <cctype>

#if defined(__APPLE__)
#include <filesystem>
#endif

#include <algorithm>

#include "utils/installdetect.h"
#include "utils/fileutil.h"
#include "utils/string_frm.h"

using namespace Tempest;
using namespace FileUtil;

static CommandLine* instance = nullptr;

static const char16_t* toString(ScriptLang lang) {
  switch(lang) {
    case ScriptLang::EN: return u"Scripts_EN";
    case ScriptLang::DE: return u"Scripts_DE";
    case ScriptLang::PL: return u"Scripts_PL";
    case ScriptLang::RU: return u"Scripts_RU";
    case ScriptLang::FR: return u"Scripts_FR";
    case ScriptLang::ES: return u"Scripts_ES";
    case ScriptLang::IT: return u"Scripts_IT";
    case ScriptLang::CZ: return u"Scripts_CZ";
    case ScriptLang::NONE:
      break;
    }
  return u"Scripts";
  }

CommandLine::CommandLine(int argc, const char** argv) {
  instance = this;
  if(argc<1)
    return;

  std::string_view mod;
  for(int i=1;i<argc;++i) {
    std::string_view arg = argv[i];
    if(arg.find("-game:")==0) {
      if(!mod.empty())
        Log::e("-game specified twice");
      mod = arg.substr(6);
      }
    else if(arg=="-g") {
      ++i;
      if(i<argc)
        gpath.assign(argv[i],argv[i]+std::strlen(argv[i]));
      }
    else if(arg=="-devmode") {
      // http://www.gothic-library.ru/publ/marvin/1-1-0-547
      devmode = true;
      }
    else if(arg=="-save") {
      ++i;
      if(i<argc){
        if(std::strcmp(argv[i],"q")==0) {
          saveDef = "save_slot_0.sav";
          } else {
          saveDef = string_frm("save_slot_",argv[i],".sav");
          }
        }
      }
    else if(arg=="-w") {
      ++i;
      if(i<argc)
        wrldDef = argv[i];
      }
    else if(arg=="-window") {
      isWindow = true;
      }
    else if(arg=="-nomenu") {
      noMenu = true;
      }
    else if(arg=="-benchmark") {
      isBenchmark = true;
      }
    else if(arg=="-g1") {
      forceG1 = true;
      }
    else if(arg=="-g2c") {
      forceG2 = true;
      }
    else if(arg=="-g2") {
      forceG2NR = true;
      }
    else if(arg=="-dx12") {
      graphics = GraphicBackend::DirectX12;
      }
    else if(arg=="-validation" || arg=="-v") {
      isDebug  = true;
      }
    else if(arg=="-rt") {
      ++i;
      if(i<argc)
        isRQuery = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="-aa") {
      ++i;
      if(i<argc) {
        try {
          aaPresetId = uint32_t(std::stoul(std::string(argv[i])));
          aaPresetId = std::clamp(aaPresetId, 0u, uint32_t(AaPreset::PRESETS_COUNT)-1u);
          }
        catch (const std::exception& e) {
          Log::i("failed to read cmaa2 preset: \"", std::string(argv[i]), "\"");
          }
        }
      }
    else if(arg=="-gi") {
      ++i;
      if(i<argc)
        isGi = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="-ms") {
      ++i;
      if(i<argc)
        isMeshSh = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="-bl") {
      // not to document - debug only
      ++i;
      if(i<argc)
        isBindlessSh = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="-vsm") {
      // not to document - debug only
      ++i;
      if(i<argc)
        isVsm = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="-rtsm") {
      // not to document - debug only
      ++i;
      if(i<argc)
        isRtSm = (std::string_view(argv[i])!="0" && std::string_view(argv[i])!="false");
      }
    else if(arg=="--vr" || arg=="-vr") {
      vr = true;
      }
    else if(arg.rfind("--vr-first-person",0)==0) {
      vrFirstPerson = true;
      auto p = arg.find('=');
      if(p!=std::string_view::npos) {
        auto val = arg.substr(p+1);
        vrFirstPerson = (val!="0" && val!="off");
      }
      }
    else if(arg.rfind("--vr-height-offset",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos) {
          vrHeight = std::stof(std::string(arg.substr(p+1)));
        } else {
          ++i;
          if(i<argc)
            vrHeight = std::stof(argv[i]);
        }
      } catch(...) {
      }
      }
    else if(arg.rfind("--vr-allow-roll",0)==0) {
      allowRoll = true;
      auto p = arg.find('=');
      if(p!=std::string_view::npos) {
        auto val = arg.substr(p+1);
        allowRoll = (val!="0" && val!="off");
      }
    }
    else if(arg.rfind("--vr-snap-deg",0)==0) {
      int snap = 30;
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos) {
          snap = std::stoi(std::string(arg.substr(p+1)));
        } else {
          ++i;
          if(i<argc)
            snap = std::stoi(argv[i]);
        }
      } catch(...) {
      }
      vrSnap = std::clamp(snap,5,90);
    }
    else if(arg.rfind("--vr-teleport",0)==0) {
      vrUseTeleport = true;
      auto p = arg.find('=');
      if(p!=std::string_view::npos) {
        auto val = arg.substr(p+1);
        vrUseTeleport = (val!="off" && val!="0");
      }
    }
    else if(arg.rfind("--vr-turn-mode",0)==0) {
      vrTurnSmooth = false;
      auto p = arg.find('=');
      if(p!=std::string_view::npos) {
        auto val = arg.substr(p+1);
        vrTurnSmooth = (val=="smooth");
      }
    }
    else if(arg.rfind("--vr-turn-speed",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrTurnSpeedVal = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrTurnSpeedVal = std::stof(argv[++i]);
      } catch(...) {}
    }
    else if(arg.rfind("--vr-turn-deadzone",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrTurnDeadzoneVal = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrTurnDeadzoneVal = std::stof(argv[++i]);
      } catch(...) {}
    }
    else if(arg.rfind("--vr-snap-cooldown-ms",0)==0) {
      int cd = vrSnapCooldownVal;
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          cd = std::stoi(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          cd = std::stoi(argv[++i]);
      } catch(...) {}
      vrSnapCooldownVal = std::clamp(cd,100,1000);
    }
    else if(arg.rfind("--vr-move-speed-scale",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrMoveScale = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrMoveScale = std::stof(argv[++i]);
      } catch(...) {}
    }
    else if(arg.rfind("--vr-vignette-strength",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrVignette = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrVignette = std::stof(argv[++i]);
      } catch(...) {}
      vrVignette = std::clamp(vrVignette,0.f,1.f);
    }
    else if(arg.rfind("--vr-render-scale",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrRenderScaleVal = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrRenderScaleVal = std::stof(argv[++i]);
      } catch(...) {}
      if(vrRenderScaleVal<0.1f) vrRenderScaleVal = 0.1f;
    }
    else if(arg.rfind("--vr-recenter-hotkey",0)==0) {
      std::string val;
      auto p = arg.find('=');
      if(p!=std::string_view::npos)
        val = std::string(arg.substr(p+1));
      else if(i+1<argc)
        val = argv[++i];
      if(!val.empty()) {
        char c = char(std::toupper(val[0]));
        switch(c) {
          case 'A': vrRecenterKeyVal = Tempest::Event::K_A; break;
          case 'B': vrRecenterKeyVal = Tempest::Event::K_B; break;
          case 'C': vrRecenterKeyVal = Tempest::Event::K_C; break;
          case 'D': vrRecenterKeyVal = Tempest::Event::K_D; break;
          case 'E': vrRecenterKeyVal = Tempest::Event::K_E; break;
          case 'F': vrRecenterKeyVal = Tempest::Event::K_F; break;
          case 'G': vrRecenterKeyVal = Tempest::Event::K_G; break;
          case 'H': vrRecenterKeyVal = Tempest::Event::K_H; break;
          case 'I': vrRecenterKeyVal = Tempest::Event::K_I; break;
          case 'J': vrRecenterKeyVal = Tempest::Event::K_J; break;
          case 'K': vrRecenterKeyVal = Tempest::Event::K_K; break;
          case 'L': vrRecenterKeyVal = Tempest::Event::K_L; break;
          case 'M': vrRecenterKeyVal = Tempest::Event::K_M; break;
          case 'N': vrRecenterKeyVal = Tempest::Event::K_N; break;
          case 'O': vrRecenterKeyVal = Tempest::Event::K_O; break;
          case 'P': vrRecenterKeyVal = Tempest::Event::K_P; break;
          case 'Q': vrRecenterKeyVal = Tempest::Event::K_Q; break;
          case 'R': vrRecenterKeyVal = Tempest::Event::K_R; break;
          case 'S': vrRecenterKeyVal = Tempest::Event::K_S; break;
          case 'T': vrRecenterKeyVal = Tempest::Event::K_T; break;
          case 'U': vrRecenterKeyVal = Tempest::Event::K_U; break;
          case 'V': vrRecenterKeyVal = Tempest::Event::K_V; break;
          case 'W': vrRecenterKeyVal = Tempest::Event::K_W; break;
          case 'X': vrRecenterKeyVal = Tempest::Event::K_X; break;
          case 'Y': vrRecenterKeyVal = Tempest::Event::K_Y; break;
          case 'Z': vrRecenterKeyVal = Tempest::Event::K_Z; break;
          default: break;
        }
      }
    }
    else if(arg.rfind("--vr-seated",0)==0) {
      std::string val;
      auto p = arg.find('=');
      if(p!=std::string_view::npos)
        val = std::string(arg.substr(p+1));
      else if(i+1<argc)
        val = argv[++i];
      vrSeatedVal = (val=="on" || val=="1" || val=="true");
    }
    else if(arg.rfind("--vr-dominant-hand",0)==0) {
      std::string val;
      auto p = arg.find('=');
      if(p!=std::string_view::npos)
        val = std::string(arg.substr(p+1));
      else if(i+1<argc)
        val = argv[++i];
      if(val=="left")
        vrDominantHandVal = VrHand::Left;
      else if(val=="right")
        vrDominantHandVal = VrHand::Right;
    }
    else if(arg.rfind("--vr-log",0)==0) {
      std::string val;
      auto p = arg.find('=');
      if(p!=std::string_view::npos)
        val = std::string(arg.substr(p+1));
      else if(i+1<argc)
        val = argv[++i];
      if(val=="off")
        vrLogLevel = VrLog::Off;
      else if(val=="verbose")
        vrLogLevel = VrLog::Verbose;
      else
        vrLogLevel = VrLog::Basic;
    }
    else if(arg.rfind("--vr-hud-distance",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrHudDist = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrHudDist = std::stof(argv[++i]);
      } catch(...) {}
    }
    else if(arg.rfind("--vr-hud-width",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrHudWidthVal = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrHudWidthVal = std::stof(argv[++i]);
      } catch(...) {}
    }
      else if(arg.rfind("--vr-hud-scale",0)==0) {
        auto p = arg.find('=');
        try {
          if(p!=std::string_view::npos)
            vrHudScaleVal = std::stof(std::string(arg.substr(p+1)));
          else if(i+1<argc)
            vrHudScaleVal = std::stof(argv[++i]);
        } catch(...) {}
        vrHudScaleVal = std::clamp(vrHudScaleVal,0.5f,2.f);
      }
    else if(arg.rfind("--vr-hud-pitch-deg",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrHudPitch = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrHudPitch = std::stof(argv[++i]);
      } catch(...) {}
    }
    else if(arg.rfind("--vr-hud-follow",0)==0) {
      auto p = arg.find('=');
      std::string v;
      if(p!=std::string_view::npos)
        v = std::string(arg.substr(p+1));
      else if(i+1<argc)
        v = argv[++i];
      if(!v.empty()) {
        vrHudFollowVal = !(v=="off" || v=="0" || v=="false");
      }
    }
    else if(arg.rfind("--vr-hud-res-scale",0)==0) {
      auto p = arg.find('=');
      try {
        if(p!=std::string_view::npos)
          vrHudRes = std::stof(std::string(arg.substr(p+1)));
        else if(i+1<argc)
          vrHudRes = std::stof(argv[++i]);
      } catch(...) {}
    }
    else {
      Log::i("unreacognized commandline option: \"", arg, "\"");
    }
    }

  if(gpath.empty()) {
    InstallDetect inst;
    gpath = inst.detectG2();
#if defined(__APPLE__)
    if(!gpath.empty() && gpath==inst.applicationSupportDirectory()) {
      std::filesystem::current_path(gpath);
      }
#endif
    }

  for(auto& i:gpath)
    if(i=='\\')
      i='/';

  if(gpath.size()>0 && gpath.back()!='/')
    gpath.push_back('/');

  gscript   = nestedPath({u"_work",u"Data",u"Scripts",   u"_compiled"},Dir::FT_Dir);
  gcutscene = nestedPath({u"_work",u"Data",u"Scripts",   u"content",u"CUTSCENE"},Dir::FT_Dir);

  gmod    = TextCodec::toUtf16(mod);
  if(!gmod.empty())
    gmod = nestedPath({u"system",gmod.c_str()},Dir::FT_File);

  if(!validateGothicPath()) {
    if(gpath.empty()) {
      Log::e("Gothic path is not provided. Please use command line argument -g <path>");
      } else {
      Log::e("Invalid gothic path: \"",TextCodec::toUtf8(gpath),"\"");
      }
    throw GothicNotFoundException("gothic not found!"); // TODO: user-friendly message-box
    }
  }

const CommandLine& CommandLine::inst() {
  assert(instance!=nullptr);
  return *instance;
  }

CommandLine::GraphicBackend CommandLine::graphicsApi() const {
  return graphics;
  }

std::u16string_view CommandLine::rootPath() const {
  return gpath;
  }

std::u16string CommandLine::scriptPath() const {
  return gscript;
  }

std::u16string CommandLine::scriptPath(ScriptLang lang) const {
  const char16_t* scripts = toString(lang);
  return nestedPath({u"_work",u"Data",scripts,u"_compiled"},Dir::FT_Dir);
  }

std::u16string CommandLine::cutscenePath() const {
  return gcutscene;
  }

std::u16string CommandLine::cutscenePath(ScriptLang lang) const {
  const char16_t* scripts = toString(lang);
  return nestedPath({u"_work",u"Data",scripts},Dir::FT_Dir);
  }

std::u16string CommandLine::nestedPath(const std::initializer_list<const char16_t*>& name, Tempest::Dir::FileType type) const {
  return FileUtil::nestedPath(gpath, name, type);
  }

bool CommandLine::isVr() const {
  return vr;
  }

bool CommandLine::validateGothicPath() const {
  if(gpath.empty())
    return false;
  if(!FileUtil::exists(gscript))
    return false;
  if(!FileUtil::exists(nestedPath({u"Data"},Dir::FT_Dir)))
    return false;
  if(!FileUtil::exists(nestedPath({u"_work",u"Data"},Dir::FT_Dir)))
    return false;
  return true;
  }
