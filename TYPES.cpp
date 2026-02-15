#pragma once

struct Vec3 {
    float x, y, z;
    Vec3 operator+(const Vec3& o) const { return {x+o.x,y+o.y,z+o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x-o.x,y-o.y,z-o.z}; }
    Vec3 operator*(float s) const { return {x*s,y*s,z*s}; }
    Vec3 operator-() const { return {-x,-y,-z}; }
    Vec3& operator+=(const Vec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x-=o.x; y-=o.y; z-=o.z; return *this; }
    Vec3& operator*=(float s) { x*=s; y*=s; z*=s; return *this; }
    float length() const { return sqrtf(x*x+y*y+z*z); }
    float lengthSq() const { return x*x+y*y+z*z; }
    Vec3 normalized() const { float l=length(); return l>1e-8f?Vec3{x/l,y/l,z/l}:Vec3{0,0,0}; }
    static Vec3 cross(const Vec3& a, const Vec3& b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
    static float dot(const Vec3& a, const Vec3& b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
};

struct Mat4 {
    float m[16];
    static Mat4 identity() { Mat4 r={}; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f; return r; }
    static Mat4 perspective(float fovY, float aspect, float zNear, float zFar) {
        Mat4 r={}; float t=tanf(fovY*0.5f);
        r.m[0]=1.0f/(aspect*t); r.m[5]=-1.0f/t;
        r.m[10]=zFar/(zNear-zFar); r.m[11]=-1.0f; r.m[14]=(zFar*zNear)/(zNear-zFar);
        return r;
    }
    static Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up) {
        Vec3 f=(center-eye).normalized(); Vec3 s=Vec3::cross(f,up).normalized(); Vec3 u=Vec3::cross(s,f);
        Mat4 r=identity();
        r.m[0]=s.x; r.m[4]=s.y; r.m[8]=s.z;
        r.m[1]=u.x; r.m[5]=u.y; r.m[9]=u.z;
        r.m[2]=-f.x; r.m[6]=-f.y; r.m[10]=-f.z;
        r.m[12]=-Vec3::dot(s,eye); r.m[13]=-Vec3::dot(u,eye); r.m[14]=Vec3::dot(f,eye);
        return r;
    }
    Mat4 operator*(const Mat4& o) const {
        Mat4 r={};
        for(int i=0;i<4;i++) for(int j=0;j<4;j++) for(int k=0;k<4;k++)
            r.m[j*4+i]+=m[k*4+i]*o.m[j*4+k];
        return r;
    }
};

struct Vertex { Vec3 pos, normal, color; };
struct PushConstants { Mat4 mvp; };

struct Fragment {
    Vec3 position, velocity, rotation, rotSpeed, color, scale;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    float lifetime, maxLifetime;
    bool eternal, active;
};

struct Block {
    Vec3 position, color;
    bool active;
    int type;
};

static float clampf(float v, float lo, float hi) { return v<lo?lo:(v>hi?hi:v); }

static const float BLOCK_SIZE=1.0f, PLAYER_HEIGHT=1.7f, PLAYER_EYE=1.6f;
static const float PLAYER_RADIUS=0.3f, GRAVITY=20.0f, JUMP_SPEED=8.0f;
static const float MOVE_SPEED=6.0f, MOUSE_SENS=0.002f, REACH_DIST=8.0f, PI=3.14159265358979f;

static std::vector<Block> worldBlocks;
static std::vector<Fragment> fragments;
static std::mt19937 rng(42);

static Vec3 playerPos={8.0f,20.0f,8.0f};
static Vec3 playerVel={0,0,0};
static float camYaw=0, camPitch=0;
static bool onGround=false;
static bool keys[256]={};
static bool mouseLocked=true;
static bool lmbDown=false;
static bool fragmentsEternal=false;
static float fragmentTimeout=10.0f;
static int targetBlockIdx=-1;
static bool hasTarget=false;

static VkInstance vkInst;
static VkPhysicalDevice physDev;
static VkDevice dev;
static VkQueue gfxQueue, presQueue;
static uint32_t gfxFam, presFam;
static VkSurfaceKHR surf;
static VkSwapchainKHR swapchain;
static std::vector<VkImage> swapImgs;
static std::vector<VkImageView> swapViews;
static VkFormat swapFmt;
static VkExtent2D swapExt;
static VkRenderPass rpass;
static VkPipelineLayout pipLayout;
static VkPipeline pipeline;
static std::vector<VkFramebuffer> fbufs;
static VkCommandPool cmdPool;
static VkCommandBuffer cmdBuf;
static VkSemaphore imgSem, renSem;
static VkFence fence;
static VkImage depImg;
static VkDeviceMemory depMem;
static VkImageView depView;
static VkBuffer vBuf=VK_NULL_HANDLE;
static VkDeviceMemory vMemory;
static VkBuffer iBuf=VK_NULL_HANDLE;
static VkDeviceMemory iMemory;
static std::vector<Vertex> allVerts;
static std::vector<uint32_t> allInds;
static bool dirty=true;
static HWND hwnd;
static int winW=1280, winH=720;
static bool running=true;
static POINT lastMouse;