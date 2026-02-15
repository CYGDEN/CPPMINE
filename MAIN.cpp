#include "IMPORT_ALL.cpp"

static void fail(const char* msg) { MessageBoxA(NULL,msg,"Error",MB_OK|MB_ICONERROR); ExitProcess(1); }

#define VK_CHECK(x) do { VkResult r=(x); if(r!=VK_SUCCESS) { char b[256]; sprintf(b,"%s failed: %d",#x,r); fail(b); } } while(0)

static std::vector<uint32_t> loadSPV(const char* path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f.is_open()) { char b[256]; sprintf(b,"Cannot open: %s",path); fail(b); }
    size_t sz=(size_t)f.tellg(); std::vector<uint32_t> buf(sz/4);
    f.seekg(0); f.read((char*)buf.data(),sz); return buf;
}

static Vec3 getCamForward() {
    return {cosf(camPitch)*sinf(camYaw),-sinf(camPitch),cosf(camPitch)*cosf(camYaw)};
}

static Vec3 getCamRight() {
    Vec3 fwd=getCamForward();
    Vec3 up={0,1,0};
    return Vec3::cross(fwd,up).normalized();
}

static Vec3 getEyePos() { return {playerPos.x,playerPos.y+PLAYER_EYE,playerPos.z}; }

static void genCube(Vec3 pos, Vec3 col, float size, std::vector<Vertex>& V, std::vector<uint32_t>& I) {
    float h=size*0.5f;
    Vec3 c[8]={
        {pos.x-h,pos.y-h,pos.z-h},{pos.x+h,pos.y-h,pos.z-h},
        {pos.x+h,pos.y+h,pos.z-h},{pos.x-h,pos.y+h,pos.z-h},
        {pos.x-h,pos.y-h,pos.z+h},{pos.x+h,pos.y-h,pos.z+h},
        {pos.x+h,pos.y+h,pos.z+h},{pos.x-h,pos.y+h,pos.z+h}
    };
    struct FD { int v[4]; Vec3 n; };
    FD faces[6]={
        {{0,3,2,1},{0,0,-1}},{{4,5,6,7},{0,0,1}},
        {{0,4,7,3},{-1,0,0}},{{1,2,6,5},{1,0,0}},
        {{0,1,5,4},{0,-1,0}},{{3,7,6,2},{0,1,0}}
    };
    uint32_t base=(uint32_t)V.size();
    for(int f=0;f<6;f++) {
        Vec3 n=faces[f].n;
        float shade;
        if(n.y>0.5f) shade=1.0f;
        else if(n.y<-0.5f) shade=0.4f;
        else if(fabsf(n.x)>0.5f) shade=0.7f;
        else shade=0.8f;
        Vec3 fc=col*shade;
        for(int v=0;v<4;v++) V.push_back({c[faces[f].v[v]],n,fc});
        uint32_t bi=base+f*4;
        I.push_back(bi); I.push_back(bi+1); I.push_back(bi+2);
        I.push_back(bi); I.push_back(bi+2); I.push_back(bi+3);
    }
}

static void genCubeHighlight(Vec3 pos, Vec3 col, float size, std::vector<Vertex>& V, std::vector<uint32_t>& I) {
    genCube(pos,col,size*1.01f,V,I);
}

static void genFragShape(Vec3 cen, Vec3 col, float bs, std::vector<Vertex>& V, std::vector<uint32_t>& I) {
    std::uniform_real_distribution<float> d(0.3f,1.0f), cd(-0.05f,0.05f), jit(-bs*0.15f,bs*0.15f);
    Vec3 fc={clampf(col.x+cd(rng),0,1),clampf(col.y+cd(rng),0,1),clampf(col.z+cd(rng),0,1)};
    float hx=bs*d(rng)*0.5f, hy=bs*d(rng)*0.5f, hz=bs*d(rng)*0.5f;
    float bx[8]={-1,1,1,-1,-1,1,1,-1}, by[8]={-1,-1,1,1,-1,-1,1,1}, bz[8]={-1,-1,-1,-1,1,1,1,1};
    Vec3 corners[8];
    for(int i=0;i<8;i++) corners[i]={cen.x+bx[i]*hx+jit(rng),cen.y+by[i]*hy+jit(rng),cen.z+bz[i]*hz+jit(rng)};
    struct FD { int v[4]; Vec3 n; };
    FD faces[6]={
        {{0,3,2,1},{0,0,-1}},{{4,5,6,7},{0,0,1}},
        {{0,4,7,3},{-1,0,0}},{{1,2,6,5},{1,0,0}},
        {{0,1,5,4},{0,-1,0}},{{3,7,6,2},{0,1,0}}
    };
    uint32_t base=(uint32_t)V.size();
    for(int f=0;f<6;f++) {
        Vec3 n=faces[f].n; float shade=0.6f+0.4f*(n.y*0.5f+0.5f); Vec3 fcc=fc*shade;
        for(int v=0;v<4;v++) V.push_back({corners[faces[f].v[v]],n,fcc});
        uint32_t bi=base+f*4;
        I.push_back(bi); I.push_back(bi+1); I.push_back(bi+2);
        I.push_back(bi); I.push_back(bi+2); I.push_back(bi+3);
    }
}

static void breakBlock(Block& bl) {
    std::uniform_real_distribution<float> pd(-0.35f, 0.35f);
    std::uniform_real_distribution<float> hd(-1.5f, 1.5f);
    std::uniform_real_distribution<float> sd(0.5f, 1.2f);
    std::uniform_real_distribution<float> rd(-1.0f, 1.0f);
    std::uniform_int_distribution<int> nf(10, 22);

    int cnt = nf(rng);
    float fs = BLOCK_SIZE / (float)cbrt((double)cnt) * 0.8f;

    for (int i = 0; i < cnt; i++) {
        Fragment fr;
        fr.position = {
            bl.position.x + pd(rng),
            bl.position.y + pd(rng),
            bl.position.z + pd(rng)
        };
        fr.velocity = {hd(rng), hd(rng) * 0.5f, hd(rng)};
        fr.rotation = {0, 0, 0};
        fr.rotSpeed = {rd(rng), rd(rng), rd(rng)};
        fr.color = bl.color;
        float s = fs * sd(rng);
        fr.scale = {s, s * sd(rng), s * sd(rng)};
        genFragShape({0, 0, 0}, bl.color, fs, fr.vertices, fr.indices);
        fr.lifetime = 0;
        fr.maxLifetime = fragmentTimeout;
        fr.eternal = fragmentsEternal;
        fr.active = true;
        fragments.push_back(fr);
    }
}

static void addBlock(Vec3 pos, Vec3 col, int type=0) {
    Block b; b.position=pos; b.color=col; b.active=true; b.type=type;
    worldBlocks.push_back(b);
}

static void generateStreet(int startX, int startZ, int length, int dir, int width) {
    Vec3 asphalt={0.15f,0.15f,0.17f};
    Vec3 sidewalk={0.45f,0.43f,0.40f};
    Vec3 curb={0.35f,0.33f,0.30f};
    Vec3 yellowLine={0.7f,0.65f,0.1f};
    for(int i=0;i<length;i++) {
        for(int w=-width;w<=width;w++) {
            int bx=startX, bz=startZ;
            if(dir==0) { bx+=i; bz+=w; } else { bz+=i; bx+=w; }
            Vec3 col=asphalt;
            if(abs(w)==width) col=curb;
            else if(abs(w)==width-1) col=sidewalk;
            else if(w==0 && (i%4<2)) col=yellowLine;
            if(abs(w)>=width-1) {
                addBlock({(float)bx,0,(float)bz},col,1);
                addBlock({(float)bx,1,(float)bz},sidewalk,1);
            } else {
                addBlock({(float)bx,0,(float)bz},col,1);
            }
        }
    }
}

static void generateBuilding(int bx, int bz, int w, int d, int h, Vec3 wallCol, Vec3 winCol, bool antenna) {
    Vec3 frame={0.25f,0.22f,0.20f};
    Vec3 roof={0.20f,0.18f,0.16f};
    Vec3 trim={0.3f,0.28f,0.25f};
    for(int x=0;x<w;x++) for(int z=0;z<d;z++) for(int y=2;y<h+2;y++) {
        bool isEdge=(x==0||x==w-1||z==0||z==d-1);
        bool isCorner=(x==0||x==w-1)&&(z==0||z==d-1);
        bool isTop=(y==h+1);
        if(!isEdge&&!isTop) continue;
        Vec3 col=wallCol;
        if(isCorner) col=frame;
        else if(isTop) col=roof;
        else if(isEdge&&!isCorner&&y>2) {
            bool isWindowRow=(y-2)%3!=0;
            bool isWindowCol;
            if(x==0||x==w-1) isWindowCol=(z>0&&z<d-1&&(z%2==1));
            else isWindowCol=(x>0&&x<w-1&&(x%2==1));
            if(isWindowRow&&isWindowCol) {
                std::uniform_real_distribution<float> lit(0.0f,1.0f);
                if(lit(rng)>0.4f) { float bright=0.6f+lit(rng)*0.4f; col={winCol.x*bright,winCol.y*bright,winCol.z*bright}; }
                else col={0.08f,0.1f,0.12f};
            } else if((y-2)%3==0) col=trim;
        }
        addBlock({(float)(bx+x),(float)y,(float)(bz+z)},col,2);
    }
    if(antenna) {
        int ax=bx+w/2, az=bz+d/2;
        for(int ay=h+2;ay<h+7;ay++) addBlock({(float)ax,(float)ay,(float)az},{0.3f,0.3f,0.3f},3);
        addBlock({(float)ax,(float)(h+7),(float)az},{0.8f,0.1f,0.1f},3);
    }
}

static void generateCity17() {
    worldBlocks.clear();

    Vec3 ground={0.2f,0.22f,0.18f};
    for(int x=-40;x<56;x++) for(int z=-40;z<56;z++) addBlock({(float)x,-1,(float)z},ground,0);

    generateStreet(-40,6,96,0,4);
    generateStreet(-40,22,96,0,4);
    generateStreet(-40,38,96,0,4);
    generateStreet(6,-40,96,1,4);
    generateStreet(22,-40,96,1,4);
    generateStreet(38,-40,96,1,4);

    Vec3 walls[]={{0.42f,0.38f,0.35f},{0.35f,0.32f,0.30f},{0.50f,0.45f,0.40f},{0.38f,0.35f,0.33f},{0.30f,0.28f,0.25f},{0.45f,0.40f,0.38f}};
    Vec3 wins[]={{0.7f,0.65f,0.3f},{0.3f,0.5f,0.7f},{0.8f,0.7f,0.4f},{0.6f,0.7f,0.8f},{0.9f,0.8f,0.5f}};
    std::uniform_int_distribution<int> hd(8,25),wd(4,8),ci(0,5),wi(0,4),ant(0,3);
    struct Lot { int x,z,w,d; };
    std::vector<Lot> lots;
    int streetPositions[]={6,22,38}; int streetWidth=9;
    for(int sx=0;sx<4;sx++) for(int sz=0;sz<4;sz++) {
        int x0=(sx==0)?-40:(streetPositions[sx-1]+streetWidth/2+1);
        int x1=(sx==3)?56:(streetPositions[sx]-streetWidth/2-1);
        int z0=(sz==0)?-40:(streetPositions[sz-1]+streetWidth/2+1);
        int z1=(sz==3)?56:(streetPositions[sz]-streetWidth/2-1);
        if(x1-x0<6||z1-z0<6) continue;
        int bw=std::min(wd(rng)+2,x1-x0-2);
        int bd=std::min(wd(rng)+2,z1-z0-2);
        lots.push_back({x0+1,z0+1,bw,bd});
        if(x1-x0>14) { int bw2=std::min(wd(rng)+2,x1-x0-bw-4); if(bw2>=4) lots.push_back({x0+1+bw+2,z0+1,bw2,bd}); }
    }
    for(auto& lot:lots) { int h=hd(rng); generateBuilding(lot.x,lot.z,lot.w,lot.d,h,walls[ci(rng)%6],wins[wi(rng)%5],ant(rng)==0); }
    generateBuilding(12,12,10,10,35,{0.30f,0.32f,0.35f},{0.5f,0.6f,0.9f},true);
    playerPos={8.0f,3.0f,8.0f};
}

static bool rayBlockIntersect(Vec3 ro, Vec3 rd, Vec3 bpos, float& t) {
    float h=0.5f;
    Vec3 mn={bpos.x-h,bpos.y-h,bpos.z-h}, mx={bpos.x+h,bpos.y+h,bpos.z+h};
    float tmin=-1e30f, tmax=1e30f;
    if(fabsf(rd.x)>1e-8f) { float t1=(mn.x-ro.x)/rd.x,t2=(mx.x-ro.x)/rd.x; if(t1>t2) std::swap(t1,t2); tmin=std::max(tmin,t1); tmax=std::min(tmax,t2); } else if(ro.x<mn.x||ro.x>mx.x) return false;
    if(fabsf(rd.y)>1e-8f) { float t1=(mn.y-ro.y)/rd.y,t2=(mx.y-ro.y)/rd.y; if(t1>t2) std::swap(t1,t2); tmin=std::max(tmin,t1); tmax=std::min(tmax,t2); } else if(ro.y<mn.y||ro.y>mx.y) return false;
    if(fabsf(rd.z)>1e-8f) { float t1=(mn.z-ro.z)/rd.z,t2=(mx.z-ro.z)/rd.z; if(t1>t2) std::swap(t1,t2); tmin=std::max(tmin,t1); tmax=std::min(tmax,t2); } else if(ro.z<mn.z||ro.z>mx.z) return false;
    if(tmin>tmax||tmax<0) return false;
    t=tmin>0?tmin:tmax; return t>0;
}

static void findTarget() {
    Vec3 eye = getEyePos(), dir = getCamForward();
    float bestT = REACH_DIST + 1;
    hasTarget = false;
    targetBlockIdx = -1;
    if (!blockGrid) return;
    float step = 0.15f;
    for (float t = 0; t < REACH_DIST; t += step) {
        Vec3 p = eye + dir * t;
        int cx = (int)floorf(p.x + 0.5f);
        int cy = (int)floorf(p.y + 0.5f);
        int cz = (int)floorf(p.z + 0.5f);
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                for (int dz = -1; dz <= 1; dz++) {
                    int idx = blockGrid->get(cx+dx, cy+dy, cz+dz);
                    if (idx < 0 || !worldBlocks[idx].active) continue;
                    Vec3 bpos = worldBlocks[idx].position;
                    float h = 0.5f;
                    Vec3 mn = {bpos.x-h, bpos.y-h, bpos.z-h};
                    Vec3 mx = {bpos.x+h, bpos.y+h, bpos.z+h};
                    float tmin = -1e30f, tmax = 1e30f;
                    bool hit = true;
                    if (fabsf(dir.x) > 1e-8f) {
                        float ta = (mn.x-eye.x)/dir.x;
                        float tb = (mx.x-eye.x)/dir.x;
                        if (ta > tb) { float tmp=ta; ta=tb; tb=tmp; }
                        tmin = std::max(tmin, ta);
                        tmax = std::min(tmax, tb);
                    } else if (eye.x < mn.x || eye.x > mx.x) hit = false;
                    if (hit) {
                        if (fabsf(dir.y) > 1e-8f) {
                            float ta = (mn.y-eye.y)/dir.y;
                            float tb = (mx.y-eye.y)/dir.y;
                            if (ta > tb) { float tmp=ta; ta=tb; tb=tmp; }
                            tmin = std::max(tmin, ta);
                            tmax = std::min(tmax, tb);
                        } else if (eye.y < mn.y || eye.y > mx.y) hit = false;
                    }
                    if (hit) {
                        if (fabsf(dir.z) > 1e-8f) {
                            float ta = (mn.z-eye.z)/dir.z;
                            float tb = (mx.z-eye.z)/dir.z;
                            if (ta > tb) { float tmp=ta; ta=tb; tb=tmp; }
                            tmin = std::max(tmin, ta);
                            tmax = std::min(tmax, tb);
                        } else if (eye.z < mn.z || eye.z > mx.z) hit = false;
                    }
                    if (hit && tmin <= tmax && tmax > 0) {
                        float ht = tmin > 0 ? tmin : tmax;
                        if (ht > 0 && ht < bestT && ht <= REACH_DIST) {
                            bestT = ht;
                            targetBlockIdx = idx;
                            hasTarget = true;
                        }
                    }
                }
            }
        }
        if (hasTarget) break;
    }
}

static bool collidesPlayerAABB(Vec3 pos) {
    float r=PLAYER_RADIUS, pMinY=pos.y, pMaxY=pos.y+PLAYER_HEIGHT;
    for(auto& bl:worldBlocks) {
        if(!bl.active) continue; float h=0.5f;
        if(pos.x+r>bl.position.x-h&&pos.x-r<bl.position.x+h&&pMaxY>bl.position.y-h&&pMinY<bl.position.y+h&&pos.z+r>bl.position.z-h&&pos.z-r<bl.position.z+h) return true;
    }
    return false;
}

static uint32_t findMem(uint32_t filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp; vkGetPhysicalDeviceMemoryProperties(physDev,&mp);
    for(uint32_t i=0;i<mp.memoryTypeCount;i++) if((filter&(1<<i))&&(mp.memoryTypes[i].propertyFlags&props)==props) return i;
    fail("No suitable memory"); return 0;
}

static void makeBuf(VkDeviceSize sz, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo bi={}; bi.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO; bi.size=sz; bi.usage=usage; bi.sharingMode=VK_SHARING_MODE_EXCLUSIVE;
    VK_CHECK(vkCreateBuffer(dev,&bi,nullptr,&buf));
    VkMemoryRequirements req; vkGetBufferMemoryRequirements(dev,buf,&req);
    VkMemoryAllocateInfo ai={}; ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; ai.allocationSize=req.size; ai.memoryTypeIndex=findMem(req.memoryTypeBits,props);
    VK_CHECK(vkAllocateMemory(dev,&ai,nullptr,&mem)); vkBindBufferMemory(dev,buf,mem,0);
}

static void destroyBuf(VkBuffer& buf, VkDeviceMemory& mem) {
    if(buf!=VK_NULL_HANDLE) { vkDestroyBuffer(dev,buf,nullptr); vkFreeMemory(dev,mem,nullptr); buf=VK_NULL_HANDLE; }
}

static void makeDepth() {
    VkImageCreateInfo ii={}; ii.sType=VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO; ii.imageType=VK_IMAGE_TYPE_2D; ii.format=VK_FORMAT_D32_SFLOAT;
    ii.extent={swapExt.width,swapExt.height,1}; ii.mipLevels=1; ii.arrayLayers=1; ii.samples=VK_SAMPLE_COUNT_1_BIT; ii.tiling=VK_IMAGE_TILING_OPTIMAL;
    ii.usage=VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; VK_CHECK(vkCreateImage(dev,&ii,nullptr,&depImg));
    VkMemoryRequirements req; vkGetImageMemoryRequirements(dev,depImg,&req);
    VkMemoryAllocateInfo ai={}; ai.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO; ai.allocationSize=req.size; ai.memoryTypeIndex=findMem(req.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(dev,&ai,nullptr,&depMem)); vkBindImageMemory(dev,depImg,depMem,0);
    VkImageViewCreateInfo vi={}; vi.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; vi.image=depImg; vi.viewType=VK_IMAGE_VIEW_TYPE_2D; vi.format=VK_FORMAT_D32_SFLOAT;
    vi.subresourceRange.aspectMask=VK_IMAGE_ASPECT_DEPTH_BIT; vi.subresourceRange.levelCount=1; vi.subresourceRange.layerCount=1;
    VK_CHECK(vkCreateImageView(dev,&vi,nullptr,&depView));
}

static void initVulkan() {
    VkApplicationInfo ai={}; ai.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO; ai.pApplicationName="City17"; ai.apiVersion=VK_API_VERSION_1_0;
    const char* ext[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    VkInstanceCreateInfo ici={}; ici.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO; ici.pApplicationInfo=&ai; ici.enabledExtensionCount=2; ici.ppEnabledExtensionNames=ext;
    VK_CHECK(vkCreateInstance(&ici,nullptr,&vkInst));
    VkWin32SurfaceCreateInfoKHR si={}; si.sType=VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR; si.hwnd=hwnd; si.hinstance=GetModuleHandle(nullptr);
    VK_CHECK(vkCreateWin32SurfaceKHR(vkInst,&si,nullptr,&surf));
    uint32_t dc=0; vkEnumeratePhysicalDevices(vkInst,&dc,nullptr); if(!dc) fail("No GPU");
    std::vector<VkPhysicalDevice> ds(dc); vkEnumeratePhysicalDevices(vkInst,&dc,ds.data()); physDev=ds[0];
    uint32_t qc=0; vkGetPhysicalDeviceQueueFamilyProperties(physDev,&qc,nullptr);
    std::vector<VkQueueFamilyProperties> qp(qc); vkGetPhysicalDeviceQueueFamilyProperties(physDev,&qc,qp.data());
    gfxFam=UINT32_MAX; presFam=UINT32_MAX;
    for(uint32_t i=0;i<qc;i++) { if(qp[i].queueFlags&VK_QUEUE_GRAPHICS_BIT) gfxFam=i; VkBool32 ps=false; vkGetPhysicalDeviceSurfaceSupportKHR(physDev,i,surf,&ps); if(ps) presFam=i; }
    if(gfxFam==UINT32_MAX||presFam==UINT32_MAX) fail("No queue families");
    float pri=1.0f; std::vector<VkDeviceQueueCreateInfo> qcis;
    VkDeviceQueueCreateInfo qi={}; qi.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO; qi.queueFamilyIndex=gfxFam; qi.queueCount=1; qi.pQueuePriorities=&pri; qcis.push_back(qi);
    if(presFam!=gfxFam) { qi.queueFamilyIndex=presFam; qcis.push_back(qi); }
    const char* de[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME}; VkPhysicalDeviceFeatures feat={};
    VkDeviceCreateInfo dci={}; dci.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO; dci.queueCreateInfoCount=(uint32_t)qcis.size(); dci.pQueueCreateInfos=qcis.data();
    dci.enabledExtensionCount=1; dci.ppEnabledExtensionNames=de; dci.pEnabledFeatures=&feat;
    VK_CHECK(vkCreateDevice(physDev,&dci,nullptr,&dev)); vkGetDeviceQueue(dev,gfxFam,0,&gfxQueue); vkGetDeviceQueue(dev,presFam,0,&presQueue);
    VkSurfaceCapabilitiesKHR caps; vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physDev,surf,&caps);
    uint32_t fc=0; vkGetPhysicalDeviceSurfaceFormatsKHR(physDev,surf,&fc,nullptr);
    std::vector<VkSurfaceFormatKHR> fmts(fc); vkGetPhysicalDeviceSurfaceFormatsKHR(physDev,surf,&fc,fmts.data());
    swapFmt=fmts[0].format; VkColorSpaceKHR cs=fmts[0].colorSpace;
    for(auto& f:fmts) { if(f.format==VK_FORMAT_B8G8R8A8_SRGB) { swapFmt=f.format; cs=f.colorSpace; break; } }
    swapExt=caps.currentExtent; if(swapExt.width==UINT32_MAX) swapExt={(uint32_t)winW,(uint32_t)winH};
    uint32_t ic=caps.minImageCount+1; if(caps.maxImageCount>0&&ic>caps.maxImageCount) ic=caps.maxImageCount;
    VkSwapchainCreateInfoKHR sci2={}; sci2.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR; sci2.surface=surf; sci2.minImageCount=ic;
    sci2.imageFormat=swapFmt; sci2.imageColorSpace=cs; sci2.imageExtent=swapExt; sci2.imageArrayLayers=1; sci2.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if(gfxFam!=presFam) { uint32_t idx[]={gfxFam,presFam}; sci2.imageSharingMode=VK_SHARING_MODE_CONCURRENT; sci2.queueFamilyIndexCount=2; sci2.pQueueFamilyIndices=idx; }
    else sci2.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
    sci2.preTransform=caps.currentTransform; sci2.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; sci2.presentMode=VK_PRESENT_MODE_FIFO_KHR; sci2.clipped=VK_TRUE;
    VK_CHECK(vkCreateSwapchainKHR(dev,&sci2,nullptr,&swapchain));
    uint32_t sic=0; vkGetSwapchainImagesKHR(dev,swapchain,&sic,nullptr); swapImgs.resize(sic); vkGetSwapchainImagesKHR(dev,swapchain,&sic,swapImgs.data());
    swapViews.resize(sic);
    for(uint32_t i=0;i<sic;i++) {
        VkImageViewCreateInfo iv={}; iv.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO; iv.image=swapImgs[i]; iv.viewType=VK_IMAGE_VIEW_TYPE_2D; iv.format=swapFmt;
        iv.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT; iv.subresourceRange.levelCount=1; iv.subresourceRange.layerCount=1;
        VK_CHECK(vkCreateImageView(dev,&iv,nullptr,&swapViews[i]));
    }
    makeDepth();
    VkAttachmentDescription att[2]={};
    att[0].format=swapFmt; att[0].samples=VK_SAMPLE_COUNT_1_BIT; att[0].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; att[0].storeOp=VK_ATTACHMENT_STORE_OP_STORE;
    att[0].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; att[0].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[0].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; att[0].finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    att[1].format=VK_FORMAT_D32_SFLOAT; att[1].samples=VK_SAMPLE_COUNT_1_BIT; att[1].loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR; att[1].storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[1].stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE; att[1].stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
    att[1].initialLayout=VK_IMAGE_LAYOUT_UNDEFINED; att[1].finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    VkAttachmentReference cref={0,VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}; VkAttachmentReference dref={1,VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
    VkSubpassDescription sub={}; sub.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS; sub.colorAttachmentCount=1; sub.pColorAttachments=&cref; sub.pDepthStencilAttachment=&dref;
    VkSubpassDependency dep={}; dep.srcSubpass=VK_SUBPASS_EXTERNAL; dep.dstSubpass=0;
    dep.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT|VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dep.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT|VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    VkRenderPassCreateInfo rpi={}; rpi.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO; rpi.attachmentCount=2; rpi.pAttachments=att;
    rpi.subpassCount=1; rpi.pSubpasses=&sub; rpi.dependencyCount=1; rpi.pDependencies=&dep;
    VK_CHECK(vkCreateRenderPass(dev,&rpi,nullptr,&rpass));
    auto vc=loadSPV("vert.spv"), frc=loadSPV("frag.spv");
    VkShaderModuleCreateInfo smi={}; smi.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO; smi.codeSize=vc.size()*4; smi.pCode=vc.data();
    VkShaderModule vm; VK_CHECK(vkCreateShaderModule(dev,&smi,nullptr,&vm));
    smi.codeSize=frc.size()*4; smi.pCode=frc.data(); VkShaderModule fm; VK_CHECK(vkCreateShaderModule(dev,&smi,nullptr,&fm));
    VkPipelineShaderStageCreateInfo stg[2]={};
    stg[0].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stg[0].stage=VK_SHADER_STAGE_VERTEX_BIT; stg[0].module=vm; stg[0].pName="main";
    stg[1].sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO; stg[1].stage=VK_SHADER_STAGE_FRAGMENT_BIT; stg[1].module=fm; stg[1].pName="main";
    VkVertexInputBindingDescription bind={}; bind.stride=sizeof(Vertex); bind.inputRate=VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription atr[3]={};
    atr[0].location=0; atr[0].format=VK_FORMAT_R32G32B32_SFLOAT; atr[0].offset=offsetof(Vertex,pos);
    atr[1].location=1; atr[1].format=VK_FORMAT_R32G32B32_SFLOAT; atr[1].offset=offsetof(Vertex,normal);
    atr[2].location=2; atr[2].format=VK_FORMAT_R32G32B32_SFLOAT; atr[2].offset=offsetof(Vertex,color);
    VkPipelineVertexInputStateCreateInfo vin={}; vin.sType=VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vin.vertexBindingDescriptionCount=1; vin.pVertexBindingDescriptions=&bind; vin.vertexAttributeDescriptionCount=3; vin.pVertexAttributeDescriptions=atr;
    VkPipelineInputAssemblyStateCreateInfo ia={}; ia.sType=VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; ia.topology=VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkViewport vp={0,0,(float)swapExt.width,(float)swapExt.height,0,1}; VkRect2D sc={{0,0},swapExt};
    VkPipelineViewportStateCreateInfo vs={}; vs.sType=VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; vs.viewportCount=1; vs.pViewports=&vp; vs.scissorCount=1; vs.pScissors=&sc;
    VkPipelineRasterizationStateCreateInfo rs={}; rs.sType=VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.polygonMode=VK_POLYGON_MODE_FILL; rs.cullMode=VK_CULL_MODE_BACK_BIT; rs.frontFace=VK_FRONT_FACE_COUNTER_CLOCKWISE; rs.lineWidth=1.0f;
    VkPipelineMultisampleStateCreateInfo ms={}; ms.sType=VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO; ms.rasterizationSamples=VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo dss={}; dss.sType=VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO; dss.depthTestEnable=VK_TRUE; dss.depthWriteEnable=VK_TRUE; dss.depthCompareOp=VK_COMPARE_OP_LESS;
    VkPipelineColorBlendAttachmentState cba={}; cba.colorWriteMask=0xF;
    VkPipelineColorBlendStateCreateInfo cb={}; cb.sType=VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; cb.attachmentCount=1; cb.pAttachments=&cba;
    VkPushConstantRange pcr={}; pcr.stageFlags=VK_SHADER_STAGE_VERTEX_BIT; pcr.size=sizeof(PushConstants);
    VkPipelineLayoutCreateInfo pli={}; pli.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; pli.pushConstantRangeCount=1; pli.pPushConstantRanges=&pcr;
    VK_CHECK(vkCreatePipelineLayout(dev,&pli,nullptr,&pipLayout));
    VkGraphicsPipelineCreateInfo gpi={}; gpi.sType=VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; gpi.stageCount=2; gpi.pStages=stg;
    gpi.pVertexInputState=&vin; gpi.pInputAssemblyState=&ia; gpi.pViewportState=&vs; gpi.pRasterizationState=&rs;
    gpi.pMultisampleState=&ms; gpi.pDepthStencilState=&dss; gpi.pColorBlendState=&cb; gpi.layout=pipLayout; gpi.renderPass=rpass;
    VK_CHECK(vkCreateGraphicsPipelines(dev,VK_NULL_HANDLE,1,&gpi,nullptr,&pipeline));
    vkDestroyShaderModule(dev,vm,nullptr); vkDestroyShaderModule(dev,fm,nullptr);
    fbufs.resize(swapViews.size());
    for(size_t i=0;i<swapViews.size();i++) {
        VkImageView a[]={swapViews[i],depView};
        VkFramebufferCreateInfo fi={}; fi.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO; fi.renderPass=rpass; fi.attachmentCount=2; fi.pAttachments=a;
        fi.width=swapExt.width; fi.height=swapExt.height; fi.layers=1;
        VK_CHECK(vkCreateFramebuffer(dev,&fi,nullptr,&fbufs[i]));
    }
    VkCommandPoolCreateInfo cpi={}; cpi.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO; cpi.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; cpi.queueFamilyIndex=gfxFam;
    VK_CHECK(vkCreateCommandPool(dev,&cpi,nullptr,&cmdPool));
    VkCommandBufferAllocateInfo cai={}; cai.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO; cai.commandPool=cmdPool; cai.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount=1;
    VK_CHECK(vkAllocateCommandBuffers(dev,&cai,&cmdBuf));
    VkSemaphoreCreateInfo semi={}; semi.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VK_CHECK(vkCreateSemaphore(dev,&semi,nullptr,&imgSem)); VK_CHECK(vkCreateSemaphore(dev,&semi,nullptr,&renSem));
    VkFenceCreateInfo fi2={}; fi2.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO; fi2.flags=VK_FENCE_CREATE_SIGNALED_BIT;
    VK_CHECK(vkCreateFence(dev,&fi2,nullptr,&fence));
}

static void rebuild() {
    allVerts.clear(); allInds.clear();
    Vec3 eye=getEyePos();
    float renderDist=80.0f;
    for(auto& bl:worldBlocks) {
        if(!bl.active) continue;
        Vec3 d=bl.position-eye;
        if(d.lengthSq()>renderDist*renderDist) continue;
        size_t startIdx=allVerts.size();
        genCubeOptimized(bl.position,bl.color,BLOCK_SIZE,allVerts,allInds);
        for(size_t i=startIdx;i<allVerts.size();i++) {
            allVerts[i].color=computeFullLighting(allVerts[i].pos,allVerts[i].normal,allVerts[i].color,eye);
        }
    }
    if(hasTarget&&targetBlockIdx>=0&&targetBlockIdx<(int)worldBlocks.size()) {
        Block& tb=worldBlocks[targetBlockIdx];
        if(tb.active) genCubeHighlight(tb.position,{1.0f,1.0f,1.0f},BLOCK_SIZE,allVerts,allInds);
    }
    for(auto& fr:fragments) {
        if(!fr.active) continue;
        uint32_t base=(uint32_t)allVerts.size();
        float cX=cosf(fr.rotation.x),sX=sinf(fr.rotation.x);
        float cY=cosf(fr.rotation.y),sY=sinf(fr.rotation.y);
        float cZ=cosf(fr.rotation.z),sZ=sinf(fr.rotation.z);
        for(auto& v:fr.vertices) {
            Vertex nv=v; Vec3 p=v.pos;
            p.x*=fr.scale.x; p.y*=fr.scale.y; p.z*=fr.scale.z;
            float y1=p.y*cX-p.z*sX,z1=p.y*sX+p.z*cX; p.y=y1; p.z=z1;
            float x2=p.x*cY+p.z*sY,z2=-p.x*sY+p.z*cY; p.x=x2; p.z=z2;
            float x3=p.x*cZ-p.y*sZ,y3=p.x*sZ+p.y*cZ; p.x=x3; p.y=y3;
            nv.pos=p+fr.position;
            nv.color=computeFullLighting(nv.pos,nv.normal,nv.color,eye);
            allVerts.push_back(nv);
        }
        for(auto idx:fr.indices) allInds.push_back(base+idx);
    }
    Vec3 right=getCamRight(), fwd=getCamForward();
    Vec3 up2=Vec3::cross(right,fwd).normalized();
    Vec3 crossPos=eye+fwd*0.3f; float cs=0.003f;
    uint32_t cb=(uint32_t)allVerts.size();
    allVerts.push_back({crossPos-right*cs-up2*cs,{0,0,1},{1,1,1}});
    allVerts.push_back({crossPos+right*cs-up2*cs,{0,0,1},{1,1,1}});
    allVerts.push_back({crossPos+right*cs+up2*cs,{0,0,1},{1,1,1}});
    allVerts.push_back({crossPos-right*cs+up2*cs,{0,0,1},{1,1,1}});
    allInds.push_back(cb); allInds.push_back(cb+1); allInds.push_back(cb+2);
    allInds.push_back(cb); allInds.push_back(cb+2); allInds.push_back(cb+3);
    if(allVerts.empty()) { allVerts.push_back({{0,0,0},{0,1,0},{0,0,0}}); allInds.push_back(0); allInds.push_back(0); allInds.push_back(0); }
}

static void uploadBufs() {
    vkDeviceWaitIdle(dev); destroyBuf(vBuf,vMemory); destroyBuf(iBuf,iMemory);
    VkDeviceSize vsz=allVerts.size()*sizeof(Vertex), isz=allInds.size()*sizeof(uint32_t);
    makeBuf(vsz,VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,vBuf,vMemory);
    void* d; vkMapMemory(dev,vMemory,0,vsz,0,&d); memcpy(d,allVerts.data(),vsz); vkUnmapMemory(dev,vMemory);
    makeBuf(isz,VK_BUFFER_USAGE_INDEX_BUFFER_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,iBuf,iMemory);
    vkMapMemory(dev,iMemory,0,isz,0,&d); memcpy(d,allInds.data(),isz); vkUnmapMemory(dev,iMemory);
}

static void physics(float dt) {
    Vec3 fwd=getCamForward(), right=getCamRight();
    Vec3 flatFwd={fwd.x,0,fwd.z}; flatFwd=flatFwd.normalized();
    Vec3 flatRight={right.x,0,right.z}; flatRight=flatRight.normalized();
    Vec3 moveDir={0,0,0};
    if(keys['W']) moveDir+=flatFwd;
    if(keys['S']) moveDir-=flatFwd;
    if(keys['A']) moveDir-=flatRight;
    if(keys['D']) moveDir+=flatRight;
    if(moveDir.lengthSq()>0.0001f) moveDir=moveDir.normalized();
    Vec3 newPos=playerPos;
    newPos.x+=moveDir.x*MOVE_SPEED*dt;
    if(collidesPlayerFast(newPos)) newPos.x=playerPos.x;
    newPos.z+=moveDir.z*MOVE_SPEED*dt;
    if(collidesPlayerFast(newPos)) newPos.z=playerPos.z;
    playerVel.y-=GRAVITY*dt;
    if(keys[VK_SPACE]&&onGround) { playerVel.y=JUMP_SPEED; onGround=false; }
    newPos.y+=playerVel.y*dt;
    if(collidesPlayerFast(newPos)) { if(playerVel.y<0) onGround=true; playerVel.y=0; newPos.y=playerPos.y; } else onGround=false;
    if(newPos.y<0) { newPos.y=0; playerVel.y=0; onGround=true; }
    playerPos=newPos;

    updateAllFragments(dt);

    findTarget(); dirty=true;
}

static void render() {
    vkWaitForFences(dev,1,&fence,VK_TRUE,UINT64_MAX); vkResetFences(dev,1,&fence);
    uint32_t idx; VkResult acq=vkAcquireNextImageKHR(dev,swapchain,UINT64_MAX,imgSem,VK_NULL_HANDLE,&idx);
    if(acq!=VK_SUCCESS) return;
    if(dirty) { rebuild(); uploadBufs(); dirty=false; }
    Vec3 eye=getEyePos(), target=eye+getCamForward();
    Mat4 view=Mat4::lookAt(eye,target,{0,1,0});
    Mat4 proj=Mat4::perspective(PI/3.0f,(float)swapExt.width/(float)swapExt.height,0.05f,500.0f);
    PushConstants pc; pc.mvp=proj*view;
    vkResetCommandBuffer(cmdBuf,0);
    VkCommandBufferBeginInfo bi={}; bi.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO; vkBeginCommandBuffer(cmdBuf,&bi);
    VkClearValue cl[2]={}; cl[0].color={{0.35f,0.38f,0.42f,1.0f}}; cl[1].depthStencil={1.0f,0};
    VkRenderPassBeginInfo rb={}; rb.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO; rb.renderPass=rpass; rb.framebuffer=fbufs[idx];
    rb.renderArea={{0,0},swapExt}; rb.clearValueCount=2; rb.pClearValues=cl;
    vkCmdBeginRenderPass(cmdBuf,&rb,VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmdBuf,VK_PIPELINE_BIND_POINT_GRAPHICS,pipeline);
    VkDeviceSize off=0; vkCmdBindVertexBuffers(cmdBuf,0,1,&vBuf,&off);
    vkCmdBindIndexBuffer(cmdBuf,iBuf,0,VK_INDEX_TYPE_UINT32);
    vkCmdPushConstants(cmdBuf,pipLayout,VK_SHADER_STAGE_VERTEX_BIT,0,sizeof(PushConstants),&pc);
    vkCmdDrawIndexed(cmdBuf,(uint32_t)allInds.size(),1,0,0,0);
    vkCmdEndRenderPass(cmdBuf); vkEndCommandBuffer(cmdBuf);
    VkPipelineStageFlags ws=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo subi={}; subi.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO; subi.waitSemaphoreCount=1; subi.pWaitSemaphores=&imgSem;
    subi.pWaitDstStageMask=&ws; subi.commandBufferCount=1; subi.pCommandBuffers=&cmdBuf; subi.signalSemaphoreCount=1; subi.pSignalSemaphores=&renSem;
    vkQueueSubmit(gfxQueue,1,&subi,fence);
    VkPresentInfoKHR pi={}; pi.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; pi.waitSemaphoreCount=1; pi.pWaitSemaphores=&renSem;
    pi.swapchainCount=1; pi.pSwapchains=&swapchain; pi.pImageIndices=&idx; vkQueuePresentKHR(presQueue,&pi);
}

static void lockMouse() {
    RECT r; GetClientRect(hwnd,&r); POINT c={(r.right-r.left)/2,(r.bottom-r.top)/2};
    ClientToScreen(hwnd,&c); SetCursorPos(c.x,c.y); lastMouse=c; ShowCursor(FALSE); mouseLocked=true;
}

static void unlockMouse() { ShowCursor(TRUE); mouseLocked=false; }

LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    switch(msg) {
    case WM_DESTROY: running=false; PostQuitMessage(0); return 0;
    case WM_KEYDOWN:
        keys[w&0xFF]=true;
        if(w==VK_F3) { fragmentsEternal=!fragmentsEternal; for(auto& f:fragments) { f.eternal=fragmentsEternal; if(!fragmentsEternal) { f.lifetime=0; f.maxLifetime=fragmentTimeout; } } }
        if(w==VK_F4) { fragments.clear(); dirty=true; }
        if(w==VK_ESCAPE) { if(mouseLocked) unlockMouse(); else { running=false; PostQuitMessage(0); } }
        return 0;
    case WM_KEYUP: keys[w&0xFF]=false; return 0;
case WM_LBUTTONDOWN:
    if(!mouseLocked) { lockMouse(); return 0; }
    if(hasTarget&&targetBlockIdx>=0&&targetBlockIdx<(int)worldBlocks.size()) {
        Block& tb=worldBlocks[targetBlockIdx];
        if(tb.active) {
            tb.active=false;
            fractureAndSpawn(tb);
            playStoneBreak();
            rebuildGrid();
            dirty=true;
        }
    }
    return 0;
    return 0;
    case WM_RBUTTONDOWN: if(!mouseLocked) lockMouse(); return 0;
    case WM_SETFOCUS: lockMouse(); return 0;
    case WM_KILLFOCUS: unlockMouse(); memset(keys,0,sizeof(keys)); lmbDown=false; return 0;
    }
    return DefWindowProc(h,msg,w,l);
}

static void cleanup() {
    cleanupGrid();
    vkDeviceWaitIdle(dev);
    vkDeviceWaitIdle(dev); destroyBuf(vBuf,vMemory); destroyBuf(iBuf,iMemory);
    vkDestroyFence(dev,fence,nullptr); vkDestroySemaphore(dev,renSem,nullptr); vkDestroySemaphore(dev,imgSem,nullptr);
    vkDestroyCommandPool(dev,cmdPool,nullptr);
    for(auto fb:fbufs) vkDestroyFramebuffer(dev,fb,nullptr);
    vkDestroyPipeline(dev,pipeline,nullptr); vkDestroyPipelineLayout(dev,pipLayout,nullptr); vkDestroyRenderPass(dev,rpass,nullptr);
    vkDestroyImageView(dev,depView,nullptr); vkDestroyImage(dev,depImg,nullptr); vkFreeMemory(dev,depMem,nullptr);
    for(auto iv:swapViews) vkDestroyImageView(dev,iv,nullptr);
    vkDestroySwapchainKHR(dev,swapchain,nullptr); vkDestroyDevice(dev,nullptr);
    vkDestroySurfaceKHR(vkInst,surf,nullptr); vkDestroyInstance(vkInst,nullptr);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE, LPSTR, int) {
    WNDCLASS wc={}; wc.lpfnWndProc=WndProc; wc.hInstance=hI; wc.lpszClassName="C17"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClass(&wc);
    hwnd=CreateWindowEx(0,"C17","[LMB:Destroy F3:Eternal F4:Clear ESC:Quit]",WS_OVERLAPPEDWINDOW|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,winW,winH,nullptr,nullptr,hI,nullptr);
    initVulkan(); initSounds(); initLighting(); generateCity17(); rebuildGrid(); dirty=true; lockMouse();
    auto lt=std::chrono::high_resolution_clock::now(); MSG msg;
    while(running) {
        while(PeekMessage(&msg,nullptr,0,0,PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        if(mouseLocked) {
            POINT cur; GetCursorPos(&cur);
            float dx=(float)(cur.x-lastMouse.x), dy=(float)(cur.y-lastMouse.y);
            camYaw-=dx*MOUSE_SENS; camPitch+=dy*MOUSE_SENS; camPitch=clampf(camPitch,-1.5f,1.5f);
            RECT r; GetClientRect(hwnd,&r); POINT c={(r.right-r.left)/2,(r.bottom-r.top)/2};
            ClientToScreen(hwnd,&c); SetCursorPos(c.x,c.y); lastMouse=c;
        }
        auto now=std::chrono::high_resolution_clock::now(); float dt=std::chrono::duration<float>(now-lt).count(); lt=now;
        if(dt>0.05f) dt=0.05f;
        physics(dt); render();
    }
    cleanup(); return 0;
}