#include "al/app/al_DistributedApp.hpp"

#include "al/sphere/al_SphereUtils.hpp"

#ifdef AL_WINDOWS
//#include <Windows.h>
#include <WinSock2.h>
#endif

using namespace al;

DistributedApp::DistributedApp() : App() {}

void DistributedApp::initialize() {
#ifdef AL_WINDOWS
  // Required to make sure gethostname() works....
  WORD wVersionRequested;
  WSADATA wsaData;
  int err;
  wVersionRequested = MAKEWORD(2, 2);
  err = WSAStartup(wVersionRequested, &wsaData);
  if (err != 0) {
    std::cerr << "WSAStartup failed with error: " << err << std::endl;
  }
#endif
  TomlLoader appConfig("distributed_app.toml");
  auto nodesTable = appConfig.root->get_table_array("node");
  std::vector<std::string> mListeners;
  // First get role from config file
  if (nodesTable) {
    for (const auto &table : *nodesTable) {
      std::string host = *table->get_as<std::string>("host");
      std::string role = *table->get_as<std::string>("role");

      if (name() == host) {
        // Now set capabilities from role
        setRole(role);
      }
      mRoleMap[host] = role;
      if (table->contains("dataRoot")) {
        if (name() == host) {  // Set configuration for this node when found
          std::string dataRootValue = *table->get_as<std::string>("dataRoot");
          dataRoot = File::conformPathToOS(dataRootValue);
        }
      } else {
        std::cout << "WARNING: node " << host.c_str() << " not given dataRoot"
                  << std::endl;
      }

      if (table->contains("rank")) {
        if (name() == host) {  // Set configuration for this node when found
          rank = *table->get_as<int>("rank");
        }
      } else {
        std::cout << "WARNING: node " << host.c_str() << " not given rank"
                  << std::endl;
      }
      if (table->contains("group")) {
        if (name() == host) {  // Set configuration for this node when found
          group = *table->get_as<int>("group");
        }
      } else {
        std::cout << "WARNING: node " << host.c_str() << " not given group"
                  << std::endl;
      }
    }
  } else {  // No nodes table in config file. Use desktop role

    auto defaultCapabilities = al::sphere::getSphereNodes();
    if (defaultCapabilities.find(name()) != defaultCapabilities.end()) {
      mCapabilites = defaultCapabilities[name()].mCapabilites;
      group = defaultCapabilities[name()].group;
      rank = defaultCapabilities[name()].rank;
    } else {
      mCapabilites =
          (Capability)(CAP_SIMULATOR | CAP_RENDERING | CAP_AUDIO_IO | CAP_OSC);
      group = 0;
    }
  }

  if (hasCapability(CAP_SIMULATOR)) {
    if (al::sphere::isSphereMachine()) {
      appConfig.setDefaultValue("broadcastAddress",
                                std::string("192.168.10.255"));
      appConfig.writeFile();
    }
  }
  if (appConfig.hasKey<std::string>("broadcastAddress")) {
    additionalConfig["broadcastAddress"] = appConfig.gets("broadcastAddress");
  }

  osc::Recv testServer;
  // probe to check if first port available, this will determine if this
  // application is the primary or the replica
  if (!testServer.open(mOSCDomain->port, mOSCDomain->interfaceIP.c_str())) {
    // If port taken, run this instance as a renderer
    mCapabilites = (Capability)(CAP_SIMULATOR | CAP_OMNIRENDERING | CAP_OSC);
    rank = 99;
    std::cout << "Replica: " << name() << ":Running distributed" << std::endl;
  } else if (rank == 0) {
    testServer.stop();
    std::cout << "Primary: " << name() << ":Running distributed" << std::endl;
  } else {
    testServer.stop();
    std::cout << "Secondary: rank " << rank << std::endl;
  }

  if (hasCapability(CAP_AUDIO_IO)) {
    mAudioControl.registerAudioIO(audioIO());
  } else {
    mDomainList.erase(
        std::find(mDomainList.begin(), mDomainList.end(), mAudioDomain));
  }
  parameterServer() << mAudioControl.gain;
}

void DistributedApp::start() {
  initialize();
  initializeDomains();
  stdControls.app = this;

  if (hasCapability(CAP_OMNIRENDERING)) {
    omniRendering =
        graphicsDomain()->newSubDomain<GLFWOpenGLOmniRendererDomain>();
    omniRendering->initialize(graphicsDomain().get());
    omniRendering->window().append(stdControls);

    omniRendering->window().append(omniRendering->navControl());
    omniRendering->navControl().nav(omniRendering->nav());
    stdControls.mWindow = &omniRendering->window();
    omniRendering->onDraw =
        std::bind(&App::onDraw, this, std::placeholders::_1);

    omniRendering->window().onKeyDown =
        std::bind(&App::onKeyDown, this, std::placeholders::_1);
    omniRendering->window().onKeyUp =
        std::bind(&App::onKeyUp, this, std::placeholders::_1);
    omniRendering->window().onMouseDown =
        std::bind(&App::onMouseDown, this, std::placeholders::_1);
    omniRendering->window().onMouseUp =
        std::bind(&App::onMouseUp, this, std::placeholders::_1);
    omniRendering->window().onMouseDrag =
        std::bind(&App::onMouseDrag, this, std::placeholders::_1);
    omniRendering->window().onMouseMove =
        std::bind(&App::onMouseMove, this, std::placeholders::_1);
    omniRendering->window().onMouseScroll =
        std::bind(&App::onMouseScroll, this, std::placeholders::_1);
    if (!isPrimary()) {
      std::cout << "Running REPLICA" << std::endl;
      omniRendering->drawOmni = true;
    }
  } else if (hasCapability(CAP_RENDERING)) {
    std::cout << "Running Primary" << std::endl;
    mDefaultWindowDomain = graphicsDomain()->newWindow();
    mDefaultWindowDomain->initialize(graphicsDomain().get());
    mDefaultWindowDomain->window().append(stdControls);
    stdControls.app = this;
    stdControls.mWindow = &mDefaultWindowDomain->window();

    defaultWindow().append(mDefaultWindowDomain->navControl());

    mDefaultWindowDomain->onDraw =
        std::bind(&App::onDraw, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onKeyDown =
        std::bind(&App::onKeyDown, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onKeyUp =
        std::bind(&App::onKeyUp, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onMouseDown =
        std::bind(&App::onMouseDown, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onMouseUp =
        std::bind(&App::onMouseUp, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onMouseDrag =
        std::bind(&App::onMouseDrag, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onMouseMove =
        std::bind(&App::onMouseMove, this, std::placeholders::_1);
    mDefaultWindowDomain->window().onMouseScroll =
        std::bind(&App::onMouseScroll, this, std::placeholders::_1);
  }

  if (isPrimary()) {
    for (auto hostRole : mRoleMap) {
      if (hostRole.first != name()) {
        parameterServer().addListener(hostRole.first, oscDomain()->port);
      }
    }
  }

  onInit();

  for (auto &domain : mDomainList) {
    mRunningDomains.push(domain);
    if (!domain->start()) {
      std::cerr << "ERROR starting domain " << std::endl;
      break;
    }
  }

  while (mRunningDomains.size() > 0) {
    if (!mRunningDomains.top()->stop()) {
      std::cerr << "ERROR stopping domain " << std::endl;
    }
    mRunningDomains.pop();
  }

  onExit();
  mDefaultWindowDomain = nullptr;
  for (auto &domain : mDomainList) {
    if (!domain->cleanup()) {
      std::cerr << "ERROR cleaning up domain " << std::endl;
    }
  }
}

std::string DistributedApp::name() { return al_get_hostname(); }

void al::DistributedApp::registerDynamicScene(DynamicScene &scene) {
  if (dynamic_cast<DistributedScene *>(&scene)) {
    // If distributed scene, connect according to this app's role
    DistributedScene *s = dynamic_cast<DistributedScene *>(&scene);
    if (isPrimary()) {
      s->registerNotifier(parameterServer());
    } else {
      parameterServer().registerOSCConsumer(s, s->name());
    }
  }

  scene.prepare(audioIO());
}

Graphics &DistributedApp::graphics() {
  if (hasCapability(CAP_OMNIRENDERING)) {
    return omniRendering->graphics();
  } else {
    return mDefaultWindowDomain->graphics();
  }
}

Window &DistributedApp::defaultWindow() {
  if (hasCapability(CAP_OMNIRENDERING)) {
    return omniRendering->window();
  } else {
    return mDefaultWindowDomain->window();
  }
}

Viewpoint &DistributedApp::view() {
  if (hasCapability(CAP_OMNIRENDERING)) {
    return omniRendering->view();
  } else {
    return mDefaultWindowDomain->view();
  }
}

Pose &DistributedApp::pose() {
  if (hasCapability(CAP_OMNIRENDERING)) {
    return omniRendering->nav();
  } else {
    return mDefaultWindowDomain->nav();
  }
}

Lens &DistributedApp::lens() { return view().lens(); }

Nav &DistributedApp::nav() {
  if (hasCapability(CAP_OMNIRENDERING)) {
    return omniRendering->nav();
  } else {
    return mDefaultWindowDomain->nav();
  }
}
