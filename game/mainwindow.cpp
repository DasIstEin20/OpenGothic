#include "mainwindow.h"

#include <Tempest/Except>
#include <Tempest/Painter>

#include <Tempest/Brush>
#include <Tempest/Pen>
#include <Tempest/Layout>
#include <Tempest/Application>
#include <Tempest/Log>
#include <Tempest/Matrix4x4>
#include <Tempest/TextureFormat>
#include <Tempest/Vec4>
#include <cmath>
#include <algorithm>
#include <Tempest/Vec3>

#include "ui/dialogmenu.h"
#include "ui/menuroot.h"
#include "ui/stacklayout.h"
#include "ui/videowidget.h"

#include "utils/mouseutil.h"
#include "utils/string_frm.h"
#include "world/triggers/abstracttrigger.h"
#include "world/objects/npc.h"
#include "game/serialize.h"
#include "game/globaleffects.h"
#include "utils/gthfont.h"
#include "utils/dbgpainter.h"

#include "commandline.h"
#include "gothic.h"

using namespace Tempest;
#ifdef OPENXR_ENABLED
#include <gapi/vulkan/vtexture.h>
#include <cmath>
namespace {
static Tempest::Vec3 rotate(const Tempest::Vec4& q, const Tempest::Vec3& v) {
  const float x = q.x, y = q.y, z = q.z, w = q.w;
  return Tempest::Vec3{
      (1.f-2.f*y*y-2.f*z*z)*v.x + 2.f*(x*y-w*z)*v.y + 2.f*(x*z+w*y)*v.z,
      2.f*(x*y+w*z)*v.x + (1.f-2.f*x*x-2.f*z*z)*v.y + 2.f*(y*z-w*x)*v.z,
      2.f*(x*z-w*y)*v.x + 2.f*(y*z+w*x)*v.y + (1.f-2.f*x*x-2.f*y*y)*v.z};
}
static Tempest::Vec3 rotateInv(const Tempest::Vec4& q, const Tempest::Vec3& v) {
  return rotate(Tempest::Vec4{-q.x,-q.y,-q.z,q.w}, v);
}
static float quatYaw(const Tempest::Vec4& q) {
  float siny_cosp = 2.f*(q.w*q.y + q.x*q.z);
  float cosy_cosp = 1.f - 2.f*(q.y*q.y + q.z*q.z);
  return std::atan2(siny_cosp, cosy_cosp);
}
static Tempest::Vec4 quatFromYawPitch(float yaw, float pitch) {
  float cy = std::cos(yaw*0.5f);
  float sy = std::sin(yaw*0.5f);
  float cp = std::cos(pitch*0.5f);
  float sp = std::sin(pitch*0.5f);
  return Tempest::Vec4{sp*cy, sy*cp, -sy*sp, cy*cp};
}
}
#endif

MainWindow::MainWindow(Device& device, IXRBackend* xr)
  : Window(Maximized),device(device),xrBackend_(xr),swapchain(device,hwnd()),
    atlas(device),renderer(swapchain),
    rootMenu(keycodec),inventory(keycodec),
    dialogs(inventory),document(keycodec),
    console(*this),
#if defined(__MOBILE_PLATFORM__)
    mobileUi(player),
#endif
    player(dialogs,inventory) {
  for(uint8_t i=0;i<Resources::MaxFramesInFlight;++i)
    fence[i] = device.fence();

  Gothic::inst().onSettingsChanged.bind(this,&MainWindow::onSettings);
  onSettings();

  if(Gothic::inst().version().game==2)
    setWindowTitle("Gothic II"); else
    setWindowTitle("Gothic");

  if(!CommandLine::inst().isWindowMode())
    setFullscreen(true);

  //renderer.resetSwapchain();
  setupUi();

  barBack    = Resources::loadTexture("BAR_BACK.TGA");
  barHp      = Resources::loadTexture("BAR_HEALTH.TGA");
  barMisc    = Resources::loadTexture("BAR_MISC.TGA");
  barMana    = Resources::loadTexture("BAR_MANA.TGA");

  focusImg   = Resources::loadTexture("FOCUS_HIGHLIGHT.TGA");

  background = Resources::loadTextureUncached("STARTSCREEN.TGA");
  loadBox    = Resources::loadTexture("PROGRESS.TGA");
  loadVal    = Resources::loadTexture("PROGRESS_BAR.TGA");

  Gothic::inst().onStartGame   .bind(this,&MainWindow::startGame);
  Gothic::inst().onLoadGame    .bind(this,&MainWindow::loadGame);
  Gothic::inst().onSaveGame    .bind(this,&MainWindow::saveGame);

  Gothic::inst().onStartLoading.bind(this,&MainWindow::onStartLoading);
  Gothic::inst().onWorldLoaded .bind(this,&MainWindow::onWorldLoaded);
  Gothic::inst().onSessionExit .bind(this,&MainWindow::onSessionExit);

  Gothic::inst().onVideo       .bind(this,&MainWindow::onVideo);

  Gothic::inst().onBenchmarkFinished.bind(this,&MainWindow::onBenchmarkFinished);

  if(!Gothic::inst().defaultSave().empty()){
    Gothic::inst().load(Gothic::inst().defaultSave());
    rootMenu.popMenu();
    }
  else if(!CommandLine::inst().doStartMenu()) {
    startGame(Gothic::inst().defaultWorld());
    rootMenu.popMenu();
    }
  else {
    rootMenu.processMusicTheme();
    }

  funcKey[2] = Shortcut(*this,Event::M_NoModifier,Event::K_F2);
  funcKey[2].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F2>);

  funcKey[3] = Shortcut(*this,Event::M_NoModifier,Event::K_F3);
  funcKey[3].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F3>);

  funcKey[4] = Shortcut(*this,Event::M_NoModifier,Event::K_F4);
  funcKey[4].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F4>);

  funcKey[5] = Shortcut(*this,Event::M_NoModifier,Event::K_F5);
  funcKey[5].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F5>);

  funcKey[6] = Shortcut(*this,Event::M_NoModifier,Event::K_F6);
  funcKey[6].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F6>);

  funcKey[7] = Shortcut(*this,Event::M_NoModifier,Event::K_F7);
  funcKey[7].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F7>);

  funcKey[9] = Shortcut(*this,Event::M_NoModifier,Event::K_F9);
  funcKey[9].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F9>);

  funcKey[10] = Shortcut(*this,Event::M_NoModifier,Event::K_F10);
  funcKey[10].onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_F10>);

  displayPos = Shortcut(*this,Event::M_Alt,Event::K_P);
  displayPos.onActivated.bind(this, &MainWindow::onMarvinKey<Event::K_P>);
  }

MainWindow::~MainWindow() {
  GameMusic::inst().stopMusic();
  Gothic::inst().cancelLoading();
  if(xrBackend_)
    xrBackend_->shutdown();
  device.waitIdle();
  takeWidget(&dialogs);
  takeWidget(&inventory);
  takeWidget(&chapter);
  takeWidget(&document);
  takeWidget(&video);
  takeWidget(&rootMenu);
#if defined(__MOBILE_PLATFORM__)
  takeWidget(&mobileUi);
#endif
  removeAllWidgets();
  // unload
  Gothic::inst().setGame(std::unique_ptr<GameSession>());
  }

float MainWindow::uiScale() const {
  return SystemApi::uiScale(hwnd());
  }

void MainWindow::setupUi() {
  setLayout(new StackLayout());
  addWidget(&document);
  addWidget(&dialogs);
  addWidget(&inventory);
  addWidget(&chapter);
  addWidget(&video);
  addWidget(&rootMenu);
#if defined(__MOBILE_PLATFORM__)
  addWidget(&mobileUi);
#endif

  rootMenu.setMainMenu();

  Gothic::inst().onDialogPipe  .bind(&dialogs,&DialogMenu::openPipe);
  Gothic::inst().isNpcInDialogFn = std::bind(&DialogMenu::isNpcInDialog, &dialogs, std::placeholders::_1);

  Gothic::inst().onPrintScreen .bind(&dialogs,&DialogMenu::printScreen);
  Gothic::inst().onPrint       .bind(&dialogs,&DialogMenu::print);

  Gothic::inst().onIntroChapter.bind(&chapter, &ChapterScreen::show);
  Gothic::inst().onShowDocument.bind(&document,&DocumentMenu::show);
  }

void MainWindow::paintEvent(PaintEvent& event) {
  Painter p(event);
  auto world = Gothic::inst().world();
  auto st    = Gothic::inst().checkLoading();

  std::string_view info;

  if(world==nullptr && !background.isEmpty()) {
    p.setBrush(Color(0.0));
    p.drawRect(0,0,w(),h());

    if(st==Gothic::LoadState::Idle) {
      p.setBrush(Brush(background,Painter::NoBlend));
      p.drawRect(0,0,w(),h(),
                 0,0,background.w(),background.h());
      }
    }

  if(world!=nullptr) {
    world->globalFx()->scrBlend(p,Rect(0,0,w(),h()));
    }

  if(st!=Gothic::LoadState::Idle && st!=Gothic::LoadState::Finalize) {
    if(st==Gothic::LoadState::Saving) {
      drawSaving(p);
      } else {
      if(auto back = Gothic::inst().loadingBanner()) {
        p.setBrush(Brush(*back,Painter::NoBlend));
        p.drawRect(0,0,this->w(),this->h(),
                   0,0,back->w(),back->h());
        }
      if(loadBox!=nullptr && !loadBox->isEmpty()) {
        if(Gothic::inst().version().game==1) {
          int lw = int(w()*0.5);
          int lh = int(h()*0.05);
          drawLoading(p,(w()-lw)/2, int(h()*0.75), lw, lh);
          } else {
          drawLoading(p,int(w()*0.92)-loadBox->w(), int(h()*0.12), loadBox->w(),loadBox->h());
          }
        }
      }
    } else {
    if(world!=nullptr && world->view()){
      auto& camera = *Gothic::inst().camera();

      auto vp = camera.viewProj();
      p.setBrush(Color(1.0));

      drawMsg(p);

      auto focus = world->validateFocus(player.focus());
      paintFocus(p,focus,vp);

      if(auto pl = Gothic::inst().player()){
        if (!Gothic::inst().isDesktop()) {
          auto& opt = Gothic::options();
          float hp  = float(pl->attribute(ATR_HITPOINTS))/float(pl->attribute(ATR_HITPOINTSMAX));
          float mp  = float(pl->attribute(ATR_MANA))     /float(pl->attribute(ATR_MANAMAX));

          bool showHealthBar = opt.showHealthBar;
          bool showManaBar   = (opt.showManaBar==2) || (opt.showManaBar==1 && (pl->weaponState()==WeaponState::Mage || inventory.isActive()));
          bool showSwimBar   = (opt.showSwimBar==2) || (opt.showSwimBar==1 && pl->isDive());

          if(showHealthBar)
            drawBar(p,barHp, 10, h()-10, hp, AlignLeft | AlignBottom);
          if(showManaBar)
            drawBar(p,barMana, w()-10, h()-10, mp, AlignRight | AlignBottom);
          if(showSwimBar) {
            uint32_t gl = pl->guild();
            auto     v  = float(pl->world().script().guildVal().dive_time[gl]);
            if(v>0) {
              auto t = float(pl->diveTime())/1000.f;
              drawBar(p,barMisc,w()/2,h()-10, (v-t)/(v), AlignHCenter | AlignBottom);
              }
            }
          }
        }
      }

    if(world && world->player()) {
      if(world->player()->hasCollision())
        info = "[c]";
      else
        info = world->roomAt(world->player()->position());
      }
    }

  if(auto c = Gothic::inst().camera()) {
    DbgPainter dbg(p,c->viewProj(),w(),h());
    c->debugDraw(dbg);
    if(world!=nullptr) {
      world->marchPoints(dbg);
      world->marchInteractives(dbg);
      world->view()->dbgLights(dbg);
      world->marchCsCameras(dbg);
      }
    }

  renderer.dbgDraw(p);

  const float scale = Gothic::interfaceScale(this);
  if(Gothic::inst().doFrate() && !Gothic::inst().isDesktop()) {
    char fpsT[64]={};
    std::snprintf(fpsT,sizeof(fpsT),"fps = %.2f",fps.get());
    //string_frm fpsT("fps = ", fps.get(), " ", info);

    auto& fnt = Resources::font(scale);
    fnt.drawText(p,5,fnt.pixelSize()+5,fpsT);
    }

  if(Gothic::inst().doClock() && world!=nullptr) {
    if (!Gothic::inst().isDesktop()) {
      auto hour = world->time().hour();
      auto min  = world->time().minute();
      auto& fnt = Resources::font(scale);
      string_frm clockT(int(hour),":",int(min));
      fnt.drawText(p,w()-fnt.textSize(clockT).w-5,fnt.pixelSize()+5,clockT);
      }
    }

  if(auto wx = Gothic::inst().worldView()) {
    wx->dbgClusters(p, Vec2(float(w()), float(h())));
    }
  }

void MainWindow::resizeEvent(SizeEvent&) {
  for(auto& i:fence)
    i.wait();
  swapchain.reset();
  renderer.resetSwapchain();
  if(auto camera = Gothic::inst().camera())
    camera->setViewport(swapchain.w(),swapchain.h());

  const bool fs = SystemApi::isFullscreen(hwnd());
  auto rect = SystemApi::windowClientRect(hwnd());
  setCursorPosition(rect.w/2,rect.h/2);
  setCursorShape(fs ? CursorShape::Hidden : CursorShape::Arrow);
  dMouse = Point();
  }

void MainWindow::mouseDownEvent(MouseEvent &event) {
  if(event.button<sizeof(mouseP))
    mouseP[event.button]=true;
  player.onKeyPressed(keycodec.tr(event),KeyEvent::K_NoKey);
  }

void MainWindow::mouseUpEvent(MouseEvent &event) {
  player.onKeyReleased(keycodec.tr(event));
  if(event.button<sizeof(mouseP))
    mouseP[event.button]=false;
  }

void MainWindow::mouseDragEvent(MouseEvent &event) {
  const bool fs = SystemApi::isFullscreen(hwnd());
  if(!mouseP[Event::ButtonLeft] && !fs)
    return;
  if(player.focus().npc && !fs)
    return;
  processMouse(event,true);
  }

void MainWindow::mouseMoveEvent(MouseEvent &event) {
  processMouse(event,SystemApi::isFullscreen(hwnd()));
  }

void MainWindow::processMouse(MouseEvent& event, bool enable) {
  auto center = Point(w()/2,h()/2);
  if(enable && event.pos()!=center && hasFocus()) {
    dMouse += (event.pos()-center);
    setCursorPosition(center);
    }
  }

void MainWindow::tickMouse() {
  auto camera = Gothic::inst().camera();
  if(dialogs.hasContent() || Gothic::inst().isPause() || camera==nullptr || camera->isCutscene()) {
    dMouse = Point();
    return;
    }

  const bool enableMouse = Gothic::inst().settingsGetI("GAME","enableMouse");
  if(enableMouse==0) {
    dMouse = Point();
    return;
    }

  const bool  camLookaroundInverse = Gothic::inst().settingsGetI("GAME","camLookaroundInverse");
  const float mouseSensitivity     = Gothic::inst().settingsGetF("GAME","mouseSensitivity")/MouseUtil::mouseSysSpeed();
  PointF dpScaled = PointF(float(dMouse.x)*mouseSensitivity,float(dMouse.y)*mouseSensitivity);
  dpScaled.x/=float(w());
  dpScaled.y/=float(h());

  dpScaled*=1000.f;
  dpScaled.y /= 7.f;
  if(camLookaroundInverse)
    dpScaled.y *= -1.f;

  camera->onRotateMouse(PointF(dpScaled.y,-dpScaled.x));
  if(!inventory.isActive()) {
    player.onRotateMouse  (-dpScaled.x);
    player.onRotateMouseDy(-dpScaled.y);
    }

  dMouse = Point();
  }

void MainWindow::onSettings() {
  auto zMaxFps = Gothic::inst().settingsGetI("ENGINE","zMaxFps");
  if(zMaxFps>0)
    maxFpsInv = 1000u/uint64_t(zMaxFps); else
    maxFpsInv = 0;
  }

void MainWindow::mouseWheelEvent(MouseEvent &event) {
  if(auto camera = Gothic::inst().camera())
    camera->changeZoom(event.delta);
  }

void MainWindow::keyDownEvent(KeyEvent &event) {
  if(video.isActive()){
    event.accept();
    video.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&video;
      return;
      }
    }

  if(rootMenu.isActive()) {
    event.accept();
    rootMenu.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&rootMenu;
      return;
      }
    }

  if(chapter.isActive()){
    event.accept();
    chapter.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&chapter;
      return;
      }
    }

  if(document.isActive()){
    event.accept();
    document.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&document;
      return;
      }
    }

  if(dialogs.isActive()){
    event.accept();
    dialogs.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&dialogs;
      return;
      }
    }

  if(inventory.isActive()){
    event.accept();
    inventory.keyDownEvent(event);
    if(event.isAccepted()){
      uiKeyUp=&inventory;
      return;
      }
    }
  uiKeyUp=nullptr;

  if(xrBackend_!=nullptr) {
    float step = CommandLine::inst().vrSnapAngle()*float(M_PI/180.f);
    if(event.key==CommandLine::inst().vrRecenterKey()) {
      xrBackend_->recenter();
      event.accept();
      return;
    }
    if(event.key==Event::K_Q) {
      vrSnapYaw -= step;
      event.accept();
      return;
    }
    if(event.key==Event::K_E) {
      vrSnapYaw += step;
      event.accept();
      return;
    }
  }

  auto act = keycodec.tr(event);
  auto mapping = keycodec.mapping(event);
  player.onKeyPressed(act,event.key,mapping);

  if(event.key==Event::K_F11) {
    auto tex = renderer.screenshoot(cmdId);
    auto pm  = device.readPixels(textureCast<const Texture2d&>(tex));
    pm.save("dbg.png");
    }
  event.accept();
}

void MainWindow::keyRepeatEvent(KeyEvent& event) {
  if(uiKeyUp==&video){
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&rootMenu){
    rootMenu.keyRepeatEvent(event);
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&chapter){
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&document){
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&dialogs){
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&inventory){
    inventory.keyRepeatEvent(event);
    if(event.isAccepted())
      return;
    }
  if(xrBackend_!=nullptr) {
    float step = CommandLine::inst().vrSnapAngle()*float(M_PI/180.f);
    if(event.key==Event::K_Q) {
      vrSnapYaw -= step;
      event.accept();
    }
    if(event.key==Event::K_E) {
      vrSnapYaw += step;
      event.accept();
    }
  }
}

void MainWindow::keyUpEvent(KeyEvent &event) {
  if(uiKeyUp==&video){
    video.keyUpEvent(event);
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&rootMenu){
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&chapter){
    chapter.keyUpEvent(event);
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&document){
    document.keyUpEvent(event);
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&dialogs){
    dialogs.keyUpEvent(event);
    if(event.isAccepted())
      return;
    }
  if(uiKeyUp==&inventory){
    inventory.keyUpEvent(event);
    if(event.isAccepted())
      return;
    }

  const char* menuEv=nullptr;

  auto act = keycodec.tr(event);
  auto mapping = keycodec.mapping(event);
  if(act==KeyCodec::Escape)
    menuEv="MENU_MAIN";
  else if(act==KeyCodec::Log)
    menuEv="MENU_LOG";
  else if(act==KeyCodec::Status)
    menuEv="MENU_STATUS";

  if(menuEv!=nullptr) {
    rootMenu.setMenu(menuEv,act);
    rootMenu.showVersion(act==KeyCodec::Escape);
    if(auto pl = Gothic::inst().player())
      rootMenu.setPlayer(*pl);
    clearInput();
    }
  else if(act==KeyCodec::Inventory && !dialogs.isActive()) {
    if(inventory.isActive()) {
      inventory.close();
      } else {
      auto pl = Gothic::inst().player();
      if(pl!=nullptr)
        inventory.open(*pl);
      }
    clearInput();
    }
  player.onKeyReleased(act, mapping);
  }

void MainWindow::focusEvent(FocusEvent &event) {
  if(!event.in)
    return;
  dMouse = Point();
  auto center = Point(w()/2,h()/2);
  setCursorPosition(center);
  }

void MainWindow::paintFocus(Painter& p, const Focus& focus, const Matrix4x4& vp) {
  if(!focus || dialogs.isActive())
    return;

  const float scale = Gothic::interfaceScale(this);
  auto        world = Gothic::inst().world();
  auto        pl    = world==nullptr ? nullptr : world->player();
  if(pl==nullptr)
    return;

  auto pos = focus.displayPosition();
  vp.project(pos.x,pos.y,pos.z);

  int   ix  = int((0.5f*pos.x+0.5f)*float(w()));
  int   iy  = int((0.5f*pos.y+0.5f)*float(h()));
  auto& fnt = Resources::font(scale);

  auto tsize = fnt.textSize(focus.displayName());
  ix-=tsize.w/2;
  if(iy<tsize.h)
    iy = tsize.h;
  if(iy>h())
    iy = h();
  fnt.drawText(p,ix,iy,focus.displayName());

  if(focus.npc!=nullptr && !focus.npc->isDead()) {
    float hp = float(focus.npc->attribute(ATR_HITPOINTS))/float(focus.npc->attribute(ATR_HITPOINTSMAX));
    drawBar(p,barHp, w()/2,10, hp, AlignHCenter|AlignTop);
    }

  const int foc = Gothic::settingsGetI("GAME","highlightMeleeFocus");
  if(focus.npc!=nullptr  &&
     (foc==1 || foc==3) &&
     player.isPressed(KeyCodec::ActionGeneric) &&
     pl->weaponState()!=WeaponState::NoWeapon &&
     pl->weaponState()!=WeaponState::Fist) {
    auto tr = vp;
    tr.mul(focus.npc->transform());

    auto b    = focus.npc->bounds();
    Vec3 bx[] = {
      {b.bbox[0].x,b.bbox[0].y,b.bbox[0].z},
      {b.bbox[1].x,b.bbox[0].y,b.bbox[0].z},
      {b.bbox[1].x,b.bbox[1].y,b.bbox[0].z},
      {b.bbox[0].x,b.bbox[1].y,b.bbox[0].z},
      {b.bbox[0].x,b.bbox[0].y,b.bbox[1].z},
      {b.bbox[1].x,b.bbox[0].y,b.bbox[1].z},
      {b.bbox[1].x,b.bbox[1].y,b.bbox[1].z},
      {b.bbox[0].x,b.bbox[1].y,b.bbox[1].z},
      };

    int min[2]={ix,iy-20}, max[2]={ix,iy-20};
    for(int i=0; i<8; ++i) {
      tr.project(bx[i]);
      int x = int((bx[i].x*0.5f+0.5f)*float(w()));
      int y = int((bx[i].y*0.5f+0.5f)*float(h()));
      min[0] = std::min(x,min[0]);
      min[1] = std::min(y,min[1]);
      max[0] = std::max(x,max[0]);
      max[1] = std::max(y,max[1]);
      }

    paintFocus(p,Rect(min[0],min[1],max[0]-min[0],max[1]-min[1]));
    }

  // focusImg
  /*
  if(auto pl = focus.interactive){
    pl->marchInteractives(p,vp,w(),h());
    }*/
  }

void MainWindow::paintFocus(Painter& p, Rect rect) {
  if(focusImg==nullptr)
    return;
  const int w2 = focusImg->w();
  const int h2 = focusImg->h();
  const int w  = w2/2;
  const int h  = h2/2;

  if(rect.w<w) {
    int dw = w-rect.w;
    rect.x -= dw/2;
    rect.w += dw;
    }
  if(rect.h<h) {
    int dh = h-rect.h;
    rect.y -= dh/2;
    rect.h += dh;
    }

  p.setBrush(Brush(*focusImg,Painter::Add));
  p.drawRect(rect.x,         rect.y,         w,h, 0,0, w, h);
  p.drawRect(rect.x+rect.w-w,rect.y,         w,h, w,0, w2,h);
  p.drawRect(rect.x,         rect.y+rect.h-h,w,h, 0,h, w, h2);
  p.drawRect(rect.x+rect.w-w,rect.y+rect.h-h,w,h, w,h, w2,h2);
  }

void MainWindow::drawBar(Painter &p, const Tempest::Texture2d* bar, int x, int y, float v, AlignFlag flg) {
  if(barBack==nullptr || bar==nullptr)
    return;
  const float scale   = Gothic::interfaceScale(this);
  const float destW   = 200.f*scale*float(std::min(w(),800))/800.f;
  const float k       = float(destW)/float(std::max(barBack->w(),1));
  const float destH   = float(barBack->h())*k;
  const float destHin = float(destH)*24.f/32.f;
  //const float destHin = 20;//float(destH)*24.f/32.f;

  v = std::max(0.f,std::min(v,1.f));
  if(flg & AlignRight)
    x-=int(destW);
  else if(flg & AlignHCenter)
    x-=int(destW)/2;
  if(flg & AlignBottom)
    y-=int(destH);

  p.setBrush(*barBack);
  p.drawRect(x,y,int(destW),int(destH), 0,0,barBack->w(),barBack->h());

  int   dy = int(0.5f*(destH-destHin));
  float pd = 9.f*k;
  p.setBrush(*bar);
  p.drawRect(x+int(pd),y+dy,int(float(destW-pd*2)*v),int(destHin),
             0,0,bar->w(),bar->h());
  }

void MainWindow::drawMsg(Tempest::Painter& p) {
  const float scale   = Gothic::interfaceScale(this);
  const float destW   = 200.f*scale*float(std::min(w(),800))/800.f;
  const float k       = float(destW)/float(std::max(barBack->w(),1));
  const float destH   = float(barBack->h())*k;

  const int y = 10 + int(destH) + 10;
  dialogs.drawMsg(p, y);
  }

void MainWindow::drawProgress(Painter &p, int x, int y, int w, int h, float v) {
  if(v<0.1f)
    v=0.1f;
  p.setBrush(*loadBox);
  p.drawRect(x,y,w,h, 0,0,loadBox->w(),loadBox->h());

  int paddL = int((float(w)*75.f)/float(loadBox->w()));
  int paddT = int((float(h)*10.f)/float(loadBox->h()));

  if(Gothic::inst().version().game==1) {
    paddL = int((float(w)*30.f)/float(loadBox->w()));
    paddT = int((float(h)* 5.f)/float(loadBox->h()));
    }

  p.setBrush(*loadVal);
  p.drawRect(x+paddL,y+paddT,int(float(w-2*paddL)*v),h-2*paddT,
             0,0,loadVal->w(),loadVal->h());
  }

void MainWindow::drawLoading(Painter &p, int x, int y, int w, int h) {
  float v = float(Gothic::inst().loadingProgress())/100.f;
  drawProgress(p,x,y,w,h,v);
  }

void MainWindow::drawSaving(Painter &p) {
  if(auto back = Gothic::inst().loadingBanner()) {
    p.setBrush(Brush(*back,Painter::NoBlend));
    p.drawRect(0,0,this->w(),this->h(),
               0,0,back->w(),back->h());
    }

  if(saveback==nullptr)
    saveback = Resources::loadTexture("SAVING.TGA");
  if(saveback==nullptr)
    return;

  const float scale = Gothic::interfaceScale(this);
  int         szX   = Gothic::options().saveGameImageSize.w;
  int         szY   = Gothic::options().saveGameImageSize.h;

  if(szX<=460 || szY<=300) {
    // way too small otherwise
    szX = 460;
    szY = 300;
    }
  szX = int(float(szX)*scale);
  szY = int(float(szY)*scale);
  drawSaving(p,*saveback,szX,szY,scale);
  }

void MainWindow::drawSaving(Painter& p, const Tempest::Texture2d& back, int sw, int sh, float scale) {
  const int x = (w()-sw)/2, y = (h()-sh)/2;

  // SAVING.TGA is semi-transparent image with the idea to accomulate alpha over time
  // ... for loop for now
  p.setBrush(back);
  for(int i=0;i<10;++i) {
    p.drawRect(x,y,sw,sh, 0,0,back.w(),back.h());
    }

  float v = float(Gothic::inst().loadingProgress())/100.f;
  drawProgress(p, x+int(100.f*scale), y+sh-int(75.f*scale), sw-2*int(100.f*scale), int(40.f*scale), v);
  }

void MainWindow::isDialogClosed(bool& ret) {
  ret = !(dialogs.isActive() || document.isActive());
  }

template<Tempest::KeyEvent::KeyType k>
void MainWindow::onMarvinKey() {
  switch(k) {
    case Event::K_F2:
      if(Gothic::inst().isMarvinEnabled()) {
        console.resize(w(),h());
        console.setFocus(true);
        console.exec();
        }
      break;
    case Event::K_F3:
      setFullscreen(!SystemApi::isFullscreen(hwnd()));
      break;
    case Event::K_F4:
      if(Gothic::inst().isMarvinEnabled()) {
        auto camera = Gothic::inst().camera();
        auto pl = Gothic::inst().player();
        if(camera!=nullptr && pl!=nullptr) {
          camera->setMarvinMode(Camera::M_Normal);
          camera->reset(pl);
          }
        }
      break;
    case Event::K_F5: {
      const bool useQuickSaveKeys = Gothic::settingsGetI("GAME", "useQuickSaveKeys")!=0;
#ifdef NDEBUG
      const bool debug = false;
#else
      const bool debug = true;
#endif
      if(!debug && Gothic::inst().isMarvinEnabled() && !dialogs.isActive()) {
        if(auto camera = Gothic::inst().camera()) {
          camera->setMarvinMode(Camera::M_Freeze);
          }
        }
      else if(Gothic::inst().isInGameAndAlive() && !Gothic::inst().isPause() && useQuickSaveKeys) {
        Gothic::inst().quickSave();
        }
      break;
      }

    case Event::K_F6:
      if(Gothic::inst().isMarvinEnabled() && !dialogs.isActive()) {
        auto camera = Gothic::inst().camera();
        auto pl     = Gothic::inst().player();
        auto inter  = pl!=nullptr ? pl->interactive() : nullptr;
        if(camera!=nullptr && inter==nullptr)
          camera->setMarvinMode(Camera::M_Free);
        }
      break;
    case Event::K_F7:
      if(Gothic::inst().isMarvinEnabled() && !dialogs.isActive()) {
        if(auto camera = Gothic::inst().camera()) {
          camera->setMarvinMode(Camera::M_Pinned);
          }
        }
      break;
    case Event::K_F8:
      //player.marvinF8();
      break;

    case Event::K_F9: {
      const bool useQuickSaveKeys = Gothic::settingsGetI("GAME", "useQuickSaveKeys")!=0;
      if(Gothic::inst().isMarvinEnabled()) {
        if(runtimeMode==R_Normal)
          runtimeMode = R_Suspended; else
          runtimeMode = R_Normal;
        }
      else if(!Gothic::inst().isPause() && useQuickSaveKeys) {
        Gothic::inst().quickLoad();
        }
      break;
      }
    case Event::K_F10:
      if(runtimeMode==R_Suspended)
        runtimeMode = R_Step;
      break;
    case Event::K_P:
      if(Gothic::inst().isMarvinEnabled()) {
        if(auto p = Gothic::inst().player()) {
          auto pos = p->position();
          string_frm buf("Position: ", pos.x,'/',pos.y,'/',pos.z);
          Gothic::inst().onPrint(buf);
          }
        }
      break;
    }
  }

uint64_t MainWindow::tick() {
  uint64_t dt = 0;
  if(xrBackend_!=nullptr) {
    if(!xrBackend_->isVisible())
      return 0;
    dt = uint64_t(xrBackend_->xrDeltaSeconds()*1000.0);
    if(dt==0)
      return 0;
    lastTick = Application::tickCount();
  } else {
    auto time = Application::tickCount();
    dt = time-lastTick;
    if(dt<5)
      return 0;
    lastTick = time;
  }

  auto st = Gothic::inst().checkLoading();
  if(st==Gothic::LoadState::Finalize || st==Gothic::LoadState::FailedLoad || st==Gothic::LoadState::FailedSave) {
    Gothic::inst().finishLoading();
    if(st==Gothic::LoadState::FailedLoad)
      rootMenu.setMainMenu();
    if(st==Gothic::LoadState::FailedSave)
      Gothic::inst().onPrint("unable to write savegame file");
    return 0;
    }
  else if(st!=Gothic::LoadState::Idle) {
    if(st==Gothic::LoadState::Loading)
      GameMusic::inst().setMusic(GameMusic::SysLoading); else
      rootMenu.processMusicTheme();
    return 0;
    }

  video.tick();
  if(video.isActive())
    return 0;

  if(Gothic::inst().isPause()) {
    return 0;
    }

  if(dt>50)
    dt=50;

  if(runtimeMode==R_Step) {
    runtimeMode = R_Suspended;
    dt = 1000/60; //60 fps
    }
  else if(runtimeMode==R_Suspended) {
    auto camera = Gothic::inst().camera();
    if(camera!=nullptr && camera->isFree()) {
      player.tickCameraMove(dt);
      tickMouse();
      }
    update();
    return dt;
    }

  dialogs.tick(dt);
  inventory.tick(dt);
  Gothic::inst().tick(dt);
  player.tickFocus();

  if(xrBackend_!=nullptr) {
    xrBackend_->pollInput();
    const auto& xr = xrBackend_->inputState();
    player.setVrMove(xr.move.x*CommandLine::inst().vrMoveSpeedScale(),
                     xr.move.y*CommandLine::inst().vrMoveSpeedScale());
    player.setVrTurn(xr.turnX);
    player.setVrJump(xr.jump);
    player.setVrAttack(xr.attack);
    player.setVrInteract(xr.interact);
    player.setVrMenu(xr.menu);

    if(xr.attack && !vrAttackPrev && CommandLine::inst().vrHaptics()) {
      auto dom = CommandLine::inst().vrDominantHand();
      IXRBackend::XRHand h = (dom==CommandLine::VrHand::Left ? IXRBackend::XRHand::Left : IXRBackend::XRHand::Right);
      xrBackend_->hapticPulse(h,0.5f,0.05f);
    }
    vrAttackPrev = xr.attack;

    if(CommandLine::inst().vrIsSmoothTurn()) {
      vrSnapYaw += xr.turnX * CommandLine::inst().vrTurnSpeed() * (float(dt)/1000.f) * float(M_PI/180.0);
    } else {
      float dz = CommandLine::inst().vrTurnDeadzone();
      if(std::fabs(xr.turnX) > dz) {
        uint64_t now = Tempest::Application::tickCount();
        if(now - vrSnapTime > uint64_t(CommandLine::inst().vrSnapCooldown())) {
          float step = CommandLine::inst().vrSnapAngle()*float(M_PI/180.f);
          vrSnapYaw += (xr.turnX>0.f?1.f:-1.f)*step;
          vrSnapTime = now;
        }
      }
    }

    if(CommandLine::inst().vrTeleport() && xr.teleportClick && !vrTelePrev && xr.aim.valid) {
      auto w = Gothic::inst().world();
      auto pl = w ? w->player() : nullptr;
      if(pl!=nullptr) {
        if(!vrNav)
          vrNav = std::make_unique<VRNav>(*w);
        Tempest::Vec3 dst;
        bool ok = vrNav->findWalkable(xr.aim.pos + xr.aim.dir*500.f, dst, CommandLine::inst().vrTeleportMaxSlope());
        if(ok) {
          if(CommandLine::inst().vrKeepHeading()) {
            float yaw = std::atan2(xr.aim.dir.x, xr.aim.dir.z)*180.f/float(M_PI);
            pl->setDirection(yaw);
          }
          pl->setPosition(dst);
          if(CommandLine::inst().vrHaptics()) {
            auto dom = CommandLine::inst().vrDominantHand();
            IXRBackend::XRHand h = (dom==CommandLine::VrHand::Left ? IXRBackend::XRHand::Left : IXRBackend::XRHand::Right);
            xrBackend_->hapticPulse(h,0.5f,0.05f);
          }
        } else if(CommandLine::inst().vrHaptics()) {
          auto dom = CommandLine::inst().vrDominantHand();
          IXRBackend::XRHand h = (dom==CommandLine::VrHand::Left ? IXRBackend::XRHand::Left : IXRBackend::XRHand::Right);
          xrBackend_->hapticPulse(h,0.15f,0.03f);
        }
      }
    }
    vrTelePrev = xr.teleportClick;
  }

  if(dialogs.isActive())
    ;//clearInput();
  if(document.isActive())
    clearInput();
  tickMouse();
  player.tickMove(dt);
  update();
  return dt;
  }

void MainWindow::updateAnimation(uint64_t dt) {
  Gothic::inst().updateAnimation(dt);
  }

void MainWindow::tickCamera(uint64_t dt) {
  auto pcamera = Gothic::inst().camera();
  auto pl      = Gothic::inst().player();
  if(pcamera==nullptr)
    return;

  auto&      camera       = *pcamera;
  const auto ws           = pl!=nullptr ? pl->weaponState() : WeaponState::NoWeapon;
  const bool meleeFocus   = (ws==WeaponState::Fist ||
                             ws==WeaponState::W1H  ||
                             ws==WeaponState::W2H);
  auto       pos          = pl!=nullptr ? pl->cameraBone(camera.isFirstPerson()) : Vec3();

  if(!camera.isCutscene()) {
    const bool fs = SystemApi::isFullscreen(hwnd());
    if(!fs && mouseP[Event::ButtonLeft]) {
      camera.setSpin(camera.destSpin());
      camera.setDestPosition(pos);
      }
    else if(dialogs.isActive() && !dialogs.isMobsiDialog()) {
      dialogs.dialogCamera(camera);
      }
    else if(inventory.isActive()) {
      camera.setDestPosition(pos);
      }
    else if(player.focus().npc!=nullptr && meleeFocus && pl!=nullptr) {
      auto spin = camera.destSpin();
      spin.y = pl->rotation();
      camera.setDestSpin(spin);
      camera.setDestPosition(pos);
      }
    else if(pl!=nullptr) {
      auto spin = camera.destSpin();
      if(pl->interactive()==nullptr && !pl->isDown())
        spin.y = pl->rotation();
      if(pl->isDive() && !camera.isMarvin())
        spin.x = -pl->rotationY();
      camera.setDestSpin(spin);
      camera.setDestPosition(pos);
      }
    }

  if(dt==0)
    return;
  if(camera.isToggleEnabled() && !camera.isCutscene())
    camera.setMode(solveCameraMode());
  camera.tick(dt);
  }

Camera::Mode MainWindow::solveCameraMode() const {
  if(auto camera = Gothic::inst().camera()) {
    if(camera->isFree())
      return Camera::Normal;
    }

  if(inventory.isOpen()==InventoryMenu::State::Equip ||
     inventory.isOpen()==InventoryMenu::State::Ransack)
    return Camera::Inventory;

  if(auto pl=Gothic::inst().player()) {
    if(pl->interactive()!=nullptr)
      return Camera::Mobsi;
    }

  if(dialogs.isActive())
    return Camera::Dialog;

  if(auto pl = Gothic::inst().player()) {
    if(pl->isDead())
      return Camera::Death;
    if(pl->isDive())
      return Camera::Dive;
    if(pl->isSwim())
      return Camera::Swim;
    if(pl->isFallingDeep())
      return Camera::Fall;
    bool g2 = Gothic::inst().version().game==2;
    switch(pl->weaponState()){
      case WeaponState::Fist:
      case WeaponState::W1H:
      case WeaponState::W2H:
        return Camera::Melee;
      case WeaponState::Bow:
      case WeaponState::CBow:
        return g2 ? Camera::Ranged : Camera::Normal;
      case WeaponState::Mage:
        return g2 ? Camera::Ranged : Camera::Melee;
      case WeaponState::NoWeapon:
        return Camera::Normal;
      }
    }

  return Camera::Normal;
  }

void MainWindow::startGame(std::string_view slot) {
  // gothic.emitGlobalSound(gothic.loadSoundFx("NEWGAME"));

  if(Gothic::inst().checkLoading()==Gothic::LoadState::Idle){
    setGameImpl(nullptr);
    }

  Gothic::inst().startLoad("LOADING.TGA",[slot=std::string(slot)](std::unique_ptr<GameSession>&& game){
    game = nullptr; // clear world-memory now
    std::unique_ptr<GameSession> w(new GameSession(slot));
    return w;
    });

  background = Texture2d();
  update();
  }

void MainWindow::loadGame(std::string_view slot) {
  if(Gothic::inst().checkLoading()==Gothic::LoadState::Idle){
    setGameImpl(nullptr);
    }

  Gothic::inst().setBenchmarkMode(false);
  Gothic::inst().startLoad("LOADING.TGA",[slot=std::string(slot)](std::unique_ptr<GameSession>&& game){
    game = nullptr; // clear world-memory now
    Tempest::RFile file(slot);
    Serialize      s(file);
    std::unique_ptr<GameSession> w(new GameSession(s));
    return w;
    });

  background = Texture2d();
  update();
  }

void MainWindow::saveGame(std::string_view slot, std::string_view name) {
  auto tex = renderer.screenshoot(cmdId);
  auto pm  = device.readPixels(textureCast<const Texture2d&>(tex));

  if(dialogs.isActive())
    return;
  if(auto w = Gothic::inst().world(); w!=nullptr && w->currentCs()!=nullptr)
    return;

  Gothic::inst().startSave(std::move(textureCast<Texture2d&>(tex)),[slot=std::string(slot),name=std::string(name),pm](std::unique_ptr<GameSession>&& game){
    if(!game)
      return std::move(game);

    Tempest::WFile f(slot);
    Serialize      s(f);
    game->save(s,name,pm);

    // no print yet, because threading
    // gothic.print("Game saved");
    return std::move(game);
    });

  update();
  }

void MainWindow::onVideo(std::string_view fname) {
  if(Gothic::inst().isBenchmarkMode())
    return;
  video.pushVideo(fname);
  }

void MainWindow::onStartLoading() {
  player   .clearInput();
  inventory.onWorldChanged();
  dialogs  .onWorldChanged();
  }

void MainWindow::onWorldLoaded() {
  dMouse = Point();

  if(Gothic::inst().isBenchmarkMode()) {
    if(auto world = Gothic::inst().world()) {
      const TriggerEvent evt("TIMEDEMO","",world->tickCount(),TriggerEvent::T_Trigger);
      world->execTriggerEvent(evt);
      }
    benchmark.clear();
    }

  player   .clearInput();
  inventory.onWorldChanged();
  dialogs  .onWorldChanged();

  device.waitIdle();
  for(auto& c:commands)
    c = device.commandBuffer();

  if(auto c = Gothic::inst().camera())
    c->setViewport(uint32_t(w()),uint32_t(h()));

  renderer.onWorldChanged();

  if(auto pl = Gothic::inst().player())
    pl->multSpeed(1.f);
  lastTick = Application::tickCount();
  player.clearFocus();
  }

void MainWindow::onSessionExit() {
  rootMenu.setMainMenu();
  }

void MainWindow::onBenchmarkFinished() {
  if(benchmark.numFrames==0)
    return;

  double fps  = benchmark.fpsSum/double(benchmark.numFrames);
  double low1 = 0;
  size_t num1 = 0;
  for(size_t i=0; i<benchmark.low1procent.size(); ++i) {
    auto v = benchmark.low1procent[i];
    if(v<=0)
      continue;
    low1 += 1000.0/double(v);
    num1 += 1;
    }
  low1 = num1>0 ? low1/double(num1) : 0.0;
  benchmark.clear();

  string_frm str("Benchmark: low 1% = ", low1, " fps = ", fps);
  Log::i(str.c_str());
  console.printLine(str);

  console.setFocus(true);
  console.exec();
  }

void MainWindow::setGameImpl(std::unique_ptr<GameSession> &&w) {
  Gothic::inst().setGame(std::move(w));
  }

void MainWindow::clearInput() {
  player.clearInput();
  std::memset(mouseP,0,sizeof(mouseP));
  }

void MainWindow::setFullscreen(bool fs) {
  SystemApi::setAsFullscreen(hwnd(),fs);
  }

void MainWindow::render(){
  try {
    static uint64_t time=Application::tickCount();

    static bool once=true;
    if(once) {
      Gothic::inst().emitGlobalSoundWav("GAMESTART.WAV");
      once=false;
      }

    /*
      Note: game update goes first
      once player position is updated, animation bones(cameraBone in particular) ca be updated
      lastly - camera position
      */
    const uint64_t dt = tick();
    updateAnimation(dt);
    tickCamera(dt);

    if(xrBackend_!=nullptr) {
      if(xrBackend_->beginFrame()) {
        auto views = xrBackend_->views();
        auto leftHand  = xrBackend_->handState(IXRBackend::XRHand::Left);
        auto rightHand = xrBackend_->handState(IXRBackend::XRHand::Right);
        float dtSec = float(xrBackend_->xrDeltaSeconds());
#ifdef OPENXR_ENABLED
        if(CommandLine::inst().vrGrab()) {
          IXRBackend::XRHandState hands[2] = {leftHand,rightHand};
          for(size_t i=0;i<2;++i) {
            auto hand = IXRBackend::XRHand(i);
            const auto& hs = hands[i];
            if(hs.isActive && hs.validGrip)
              player.vrUpdateGrab(hand, hs.gripPose, dtSec);
            else
              player.vrReleaseGrab(hand);
            bool sq = hs.squeeze;
            if(hs.isActive && hs.validAim) {
              if(sq && !vrSqueezePrev[i]) {
                float x=0,y=0,z=0,w=1;
                hs.aimPose.project(x,y,z,w);
                Tempest::Vec3 org{x/w,y/w,z/w};
                x=0; y=0; z=-1; w=1;
                hs.aimPose.project(x,y,z,w);
                Tempest::Vec3 fwd{x/w,y/w,z/w};
                Tempest::Vec3 dir = fwd - org;
                if(player.vrTryGrab(hand, org, dir)) {
                  if(CommandLine::inst().vrHaptics())
                    xrBackend_->hapticPulse(hand,0.6f,0.06f);
                }
              }
              if(!sq && vrSqueezePrev[i]) {
                player.vrReleaseGrab(hand);
                if(CommandLine::inst().vrHaptics())
                  xrBackend_->hapticPulse(hand,0.3f,0.04f);
              }
              vrSqueezePrev[i] = sq;
            } else {
              if(vrSqueezePrev[i]) {
                player.vrReleaseGrab(hand);
                vrSqueezePrev[i] = false;
              }
            }
          }
        }
#endif
        if(vrSnapYaw!=0.f) {
          Matrix4x4 rot;
          rot.identity();
          rot.rotateOY(vrSnapYaw);
          for(auto& eye:views) {
            Matrix4x4 m = rot;
            m.mul(eye.view);
            eye.view = m;
          }
        }

        if(auto cam = Gothic::inst().camera()) {
          if(CommandLine::inst().isVrFirstPerson())
            cam->setExternalViewProj(&views[0].view,&views[0].proj);
          else
            cam->clearExternalViewProj();
        }

        auto& sync = fence[cmdId];
        if(!sync.wait(0)) {
          std::this_thread::yield();
          xrBackend_->endFrame();
          if(auto cam = Gothic::inst().camera())
            cam->clearExternalViewProj();
          return;
        }
        Resources::resetRecycled(cmdId);

        if(video.isActive()) {
          video.paint(device,cmdId);
          uiLayer.clear();
          PaintEvent p(uiLayer,atlas,this->w(),this->h());
          video.paintEvent(p);
        } else if(needToUpdate() || Gothic::inst().checkLoading()!=Gothic::LoadState::Idle) {
          dispatchPaintEvent(uiLayer,atlas);

          numOverlay.clear();
          PaintEvent p(numOverlay,atlas,this->w(),this->h());
          inventory.paintNumOverlay(p);
        }
        uiMesh [cmdId].update(device,uiLayer);
        numMesh[cmdId].update(device,numOverlay);

        int hudW = int(1920 * CommandLine::inst().vrHudResScale());
        int hudH = int(1080 * CommandLine::inst().vrHudResScale());
        if(vrHud.isEmpty() || vrHud.w()!=uint32_t(hudW) || vrHud.h()!=uint32_t(hudH)) {
          vrHud = device.attachment(Tempest::TextureFormat::RGBA8, hudW, hudH);
          vrHudDepth = device.zbuffer(Tempest::TextureFormat::Depth16, hudW, hudH);
        }

        CommandBuffer& cmd = commands[cmdId];
        {
          auto enc = cmd.startEncoding(device);
          for(auto& eye:views) {
            renderer.draw(eye.color, enc, cmdId);
            if(CommandLine::inst().vrShowHands() && xrBackend_->isVisible()) {
              Tempest::Matrix4x4 viewProj = eye.proj;
              viewProj.mul(eye.view);
              float scale = CommandLine::inst().vrHandScale();

              auto drawHand = [&](const IXRBackend::XRHandState& st, const Tempest::Vec3& clr) {
                if(!st.isActive || !st.validGrip)
                  return;
                Tempest::Matrix4x4 m = st.gripPose;
                m.scale(scale);
                renderer.drawBox(eye.color, enc, viewProj, m, clr);
              };

              drawHand(leftHand,  CommandLine::inst().vrHandColorLeft());
              drawHand(rightHand, CommandLine::inst().vrHandColorRight());

              if(CommandLine::inst().vrLaser()) {
                auto dom = CommandLine::inst().vrDominantHand();
                const auto& hs = (dom==CommandLine::VrHand::Left ? leftHand : rightHand);
                Tempest::Vec3 lclr = (dom==CommandLine::VrHand::Left ? CommandLine::inst().vrHandColorLeft() : CommandLine::inst().vrHandColorRight());
                if(hs.isActive && hs.validAim) {
                  float x=0,y=0,z=0,w=1;
                  hs.aimPose.project(x,y,z,w);
                  Tempest::Vec3 origin{x/w,y/w,z/w};
                  x=0; y=0; z=-1; w=1;
                  hs.aimPose.project(x,y,z,w);
                  Tempest::Vec3 fwd{x/w,y/w,z/w};
                  Tempest::Vec3 dir = fwd - origin;
                  Tempest::Vec3 end = origin + dir*10.f;
                  renderer.drawLine(eye.color, enc, viewProj, origin, end, lclr);
                  Tempest::Matrix4x4 qm;
                  qm.identity();
                  qm.translate(end.x,end.y,end.z);
                  qm.scale(scale*0.05f);
                  renderer.drawQuad(eye.color, enc, viewProj, qm, lclr);
                }
              }
            }
          }
          enc.setFramebuffer({{vrHud, Tempest::Discard, Tempest::Preserve}});
          enc.setDebugMarker("VR-HUD");
          uiMesh[cmdId].draw(enc);
          if(inventory.isOpen()!=InventoryMenu::State::Closed) {
            enc.setFramebuffer({{vrHud, Tempest::Preserve, Tempest::Preserve}},{vrHudDepth,1.f,Tempest::Preserve});
            inventory.draw(enc);
            enc.setFramebuffer({{vrHud, Tempest::Preserve, Tempest::Preserve}});
            numMesh[cmdId].draw(enc);
          }
        }
        device.submit(cmd,sync);

        HudImageInfo imgInfo{};
        if(getVrHudImage(imgInfo)) {
          float hudScale = CommandLine::inst().vrHudScale();
          float hudMeters = CommandLine::inst().vrHudWidth()*hudScale;
          Tempest::Vec4 headQ = xrBackend_->headOrientation();
          Tempest::Vec3 headPos = xrBackend_->headPosition();
          float targetYaw = quatYaw(headQ);
          float dist = CommandLine::inst().vrHudDistance();
          float pitch = CommandLine::inst().vrHudPitchDeg()*float(M_PI)/180.f;
          Tempest::Vec3 forward = rotate(quatFromYawPitch(targetYaw,0.f), Tempest::Vec3{0,0,-1});
          Tempest::Vec3 targetPos = headPos + forward*dist;
          if(!hudInitialized) {
            hudPos = targetPos;
            hudYaw = targetYaw;
            hudInitialized = true;
          } else if(CommandLine::inst().vrHudFollow()) {
            hudPos += (targetPos-hudPos)*0.15f;
            float dy = targetYaw - hudYaw;
            if(dy>float(M_PI)) dy-=float(2*M_PI);
            if(dy<-float(M_PI)) dy+=float(2*M_PI);
            hudYaw += dy*0.15f;
          }
          Tempest::Vec4 hudQ = quatFromYawPitch(hudYaw, pitch);
          IXRBackend::XRQuadLayerDesc hud{};
          hud.image       = imgInfo.image;
          hud.width       = imgInfo.w;
          hud.height      = imgInfo.h;
          hud.metersWidth = hudMeters;
          hud.pose.orientation = {hudQ.x, hudQ.y, hudQ.z, hudQ.w};
          hud.pose.position    = {hudPos.x, hudPos.y, hudPos.z};
          handleVrPointer(hud);
          xrBackend_->setUiQuad(&hud);
          xrBackend_->endFrame();
        } else {
          xrBackend_->endFrame();
        }
        if(auto cam = Gothic::inst().camera())
          cam->clearExternalViewProj();
        cmdId = (cmdId+1u)%Resources::MaxFramesInFlight;
        return;
      } else {
        if(!xrBackend_->isRunning()) {
          xrBackend_->shutdown();
          delete xrBackend_;
          xrBackend_ = nullptr;
          vrSnapYaw = 0.f;
        }
        return;
      }
    }

    auto& sync = fence[cmdId];
    if(!sync.wait(0)) {
      // GPU rendering is not done, pass to next frame
      std::this_thread::yield();
      return;
      }
    Resources::resetRecycled(cmdId);

    if(video.isActive()) {
      video.paint(device,cmdId);
      uiLayer.clear();
      PaintEvent p(uiLayer,atlas,this->w(),this->h());
      video.paintEvent(p);
      }
    else if(needToUpdate() || Gothic::inst().checkLoading()!=Gothic::LoadState::Idle) {
      dispatchPaintEvent(uiLayer,atlas);

      numOverlay.clear();
      PaintEvent p(numOverlay,atlas,this->w(),this->h());
      inventory.paintNumOverlay(p);
      }
    uiMesh [cmdId].update(device,uiLayer);
    numMesh[cmdId].update(device,numOverlay);

    CommandBuffer& cmd = commands[cmdId];
    {
    auto enc = cmd.startEncoding(device);
    renderer.draw(enc,cmdId,swapchain.currentImage(),uiMesh[cmdId],numMesh[cmdId],inventory,video);
    }
    device.submit(cmd,sync);
    device.present(swapchain);
    cmdId = (cmdId+1u)%Resources::MaxFramesInFlight;

    auto t = Application::tickCount();
    if(t-time<16 && !Gothic::inst().isInGame() && !video.isActive()) {
      uint32_t delay = uint32_t(16-(t-time));
      Application::sleep(delay);
      t += delay;
      }
    else if(maxFpsInv>0 && t-time<maxFpsInv) {
      uint32_t delay = uint32_t(maxFpsInv-(t-time));
      Application::sleep(delay);
      t += delay;
      }
    fps.push(t-time);
    if(Gothic::inst().isBenchmarkMode() && Gothic::inst().world()!=nullptr && Gothic::inst().world()->currentCs()!=nullptr)
      benchmark.push(t-time);
    time = t;
    }
  catch(const Tempest::SwapchainSuboptimal&) {
    Log::e("swapchain is outdated - reset renderer");
    device.waitIdle();
    swapchain.reset();
    renderer.resetSwapchain();
    }
  }

double MainWindow::Fps::get() const {
  uint64_t sum=0,num=0;
  for(auto& i:dt)
    if(i>0) {
      sum+=i;
      num++;
      }
  if(num==0 || sum==0)
    return 60;
  uint64_t fps = (1000*100*num)/sum;
  return double(fps)/100.0;
  }

void MainWindow::Fps::push(uint64_t t) {
  for(size_t i=9;i>0;--i)
    dt[i]=dt[i-1];
  dt[0]=t;
  }

void MainWindow::Benchmark::push(uint64_t t) {
  fpsSum += t>0 ? (1000.0/double((t))) : 60.0;
  numFrames++;
  auto at = std::lower_bound(low1procent.begin(), low1procent.end(), t, std::greater<uint64_t>());
  low1procent.insert(at, t);
  low1procent.resize(std::min(low1procent.size(), (numFrames+99)/100));
  }

void MainWindow::Benchmark::clear() {
  low1procent.reserve(128);
  low1procent.clear();
  numFrames = 0;
  fpsSum = 0;
  }

#ifdef OPENXR_ENABLED
bool MainWindow::getVrHudImage(HudImageInfo& out) const {
  if(vrHud.isEmpty())
    return false;
  auto& tex = textureCast<const Texture2d&>(vrHud);
  auto* vkTex = reinterpret_cast<Tempest::Detail::VTexture*>(tex.impl.handler);
  out.image  = vkTex->impl;
  out.w      = tex.w();
  out.h      = tex.h();
  return true;
}
#endif

#ifdef OPENXR_ENABLED
void MainWindow::handleVrPointer(const IXRBackend::XRQuadLayerDesc& hud) {
  const auto& st = xrBackend_->inputState();
  if(!st.aim.valid) {
    if(vrPointerPressed && !st.interact) {
      Tempest::MouseEvent mu(0,0,Tempest::MouseEvent::ButtonLeft);
      mouseUpEvent(mu);
      vrPointerPressed=false;
    }
    vrPointerPressTime = 0;
    return;
  }
  Tempest::Vec4 q{hud.pose.orientation.x, hud.pose.orientation.y, hud.pose.orientation.z, hud.pose.orientation.w};
  Tempest::Vec3 pos{hud.pose.position.x, hud.pose.position.y, hud.pose.position.z};
  Tempest::Vec3 normal = rotate(q, Tempest::Vec3{0.f,0.f,1.f});
  float denom = Tempest::Vec3::dotProduct(st.aim.dir, normal);
  if(std::fabs(denom) < 1e-6f)
    return;
  float t = Tempest::Vec3::dotProduct(pos - st.aim.pos, normal)/denom;
  if(t<=0.f)
    return;
  Tempest::Vec3 hit = st.aim.pos + st.aim.dir*t;
  Tempest::Vec3 local = rotateInv(q, hit - pos);
  float widthM  = hud.metersWidth;
  float heightM = hud.metersWidth * float(hud.height)/float(hud.width);
  float u = local.x/widthM + 0.5f;
  float v = -local.y/heightM + 0.5f;
  if(u<0.f || u>1.f || v<0.f || v>1.f) {
    if(vrPointerPressed) {
      Tempest::MouseEvent mu(0,0,Tempest::MouseEvent::ButtonLeft);
      mouseUpEvent(mu);
      vrPointerPressed=false;
    }
    return;
  }
  int px = int(u*float(hud.width));
  int py = int(v*float(hud.height));
  if(vrPointerPressed) {
    Tempest::MouseEvent mv(px,py,Tempest::MouseEvent::ButtonLeft);
    mouseMoveEvent(mv);
    mouseDragEvent(mv);
  } else {
    Tempest::MouseEvent mv(px,py,Tempest::MouseEvent::ButtonNone);
    mouseMoveEvent(mv);
  }

  auto w = Gothic::inst().world();
  uint64_t now = w ? w->tickCount() : 0;
  if(st.interact && !vrPointerPressed) {
    Tempest::MouseEvent md(px,py,Tempest::MouseEvent::ButtonLeft);
    mouseDownEvent(md);
    vrPointerPressed=true;
    vrPointerPressTime = now;
  } else if(!st.interact && vrPointerPressed) {
    if(vrPointerPressTime!=0 && now - vrPointerPressTime > uint64_t(CommandLine::inst().vrUiLongPress()*1000.f)) {
      Tempest::MouseEvent md(px,py,Tempest::MouseEvent::ButtonRight);
      mouseDownEvent(md);
      mouseUpEvent(md);
      Tempest::MouseEvent mu(px,py,Tempest::MouseEvent::ButtonLeft);
      mouseUpEvent(mu);
    } else {
      Tempest::MouseEvent mu(px,py,Tempest::MouseEvent::ButtonLeft);
      mouseUpEvent(mu);
    }
    vrPointerPressed=false;
    vrPointerPressTime = 0;
  }

  vrPointerX = px;
  vrPointerY = py;

  float scr = st.move.y;
  if(std::fabs(scr) > 0.1f) {
    Tempest::MouseEvent mw(px,py,Tempest::MouseEvent::ButtonNone);
    mw.delta = int(scr * CommandLine::inst().vrUiScrollScale());
    mouseWheelEvent(mw);
  }
}
#endif
