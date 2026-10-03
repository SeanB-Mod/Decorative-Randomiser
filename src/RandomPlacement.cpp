#include "common.h"
#include "il2cpp_array_layout.h"
#include "random_logic.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

namespace mod {
struct Api {
    void* (*domain_get)(); void* (*thread_current)(); const void** (*domain_get_assemblies)(void*,size_t*);
    const void* (*assembly_get_image)(const void*); void* (*class_from_name)(const void*,const char*,const char*);
    void* (*class_get_parent)(void*); const void* (*class_get_method_from_name)(void*,const char*,int);
    const void* (*class_get_field_from_name)(void*,const char*); void* (*object_get_class)(void*);
    void* (*runtime_invoke)(const void*,void*,void**,void**); const void* (*class_get_type)(void*);
    void* (*type_get_object)(const void*); void* (*object_unbox)(void*); void (*field_get_value)(void*,const void*,void*);
    void (*field_static_get_value)(const void*,void*); void (*field_set_value)(void*,const void*,void*); void (*field_set_value_object)(void*,const void*,void*);
    uintptr_t (*gchandle_new)(void*,bool); void (*gchandle_free)(uintptr_t); uintptr_t (*array_length)(void*);
    void* (*object_new)(void*); void* (*array_new)(void*,uintptr_t); void* (*string_new)(const char*);
    const wchar_t* (*string_chars)(void*); int32_t (*string_length)(void*);
    const void* (*class_get_methods)(void*,void**); const char* (*method_get_name)(const void*);
    unsigned (*method_get_param_count)(const void*); const void* (*method_get_param)(const void*,unsigned);
    char* (*type_get_name)(const void*); void (*free)(void*);
} api{};

struct V2{float x,y;}; struct V3{float x,y,z;}; struct Rect{float x,y,w,h;}; struct Color{float r,g,b,a;};
struct Color32{unsigned char r,g,b,a;};
struct Tint{Color color;int property;};
struct NullableTint{bool hasValue;unsigned char pad[3];Tint value;};
struct TooltipText{void* dev;int term;int padding;};
static_assert(sizeof(NullableTint)==24);

static HWND window=nullptr; static DWORD windowThread=0; static UINT messageId=0; static std::atomic<bool> stopping{false};
static HANDLE logFile=INVALID_HANDLE_VALUE; static SRWLOCK logLock=SRWLOCK_INIT; static bool initialized=false,failed=false;
static void* itemsType=nullptr,*foundationsType=nullptr,*roomsType=nullptr; static uintptr_t itemsTypeHandle=0,foundationsTypeHandle=0,roomsTypeHandle=0; static const void *findAll=nullptr,*aliveMethod=nullptr,*gameObjectMethod=nullptr,*activeMethod=nullptr,*openMethod=nullptr;
static void *cursorClass=nullptr,*placeClass=nullptr,*tintProxyClass=nullptr;
static void *buildLogicClass=nullptr; static const void *buildLogicInstanceField=nullptr;
static const void *cursorInstanceField=nullptr,*cursorModeField=nullptr,*placeItemField=nullptr,*placeEditModeField=nullptr,*placeDefinitionField=nullptr,*placeDesiredRotationField=nullptr,*placeRotateControlField=nullptr,*placeTintColorsField=nullptr,*isFreePlacementMethod=nullptr,*mouseDownField=nullptr,*objectNameMethod=nullptr;
static void* itemsMenu=nullptr,*demolitionTarget=nullptr,*demolitionObjects[2]{},*demolitionButtons[2]{},*buttons[3]{},*nativeButtons[3]{},*childGraphics[3]{},*icons[3]{},*tips[3]{}; static std::vector<uintptr_t> holds;
static void* toolbarDonorObject=nullptr,*toolbarDonorTransform=nullptr,*toolbarOverlayParent=nullptr; static bool toolbarVisible=false,toolbarPositionValid=false; static V3 lastToolbarWorldPosition{};
static void* gameIcons[3]{},*offIcons[3]{}; static uintptr_t gameIconHandles[3]{},offIconHandles[3]{}; static bool gameIconsResolved=false;
static bool flags[3]{false,false,false}; static bool lastFlags[3]{false,false,false}; static bool previousDown[3]{false,false,false},previousDemolitionDown[2]{false,false}; static void* lastItem=nullptr;
static std::wstring iniPath; static random_logic::Rng rng(static_cast<std::uint64_t>(GetTickCount64())^reinterpret_cast<uintptr_t>(&api)); static random_logic::HueBag hueBag;

static void log(const char* s){AcquireSRWLockExclusive(&logLock);if(logFile!=INVALID_HANDLE_VALUE){DWORD n;WriteFile(logFile,s,(DWORD)strlen(s),&n,nullptr);}ReleaseSRWLockExclusive(&logLock);}
static void* klass(const char* ns,const char* name){size_t count=0;auto a=api.domain_get_assemblies(api.domain_get(),&count);if(!a||count>4096)return nullptr;for(size_t i=0;i<count;i++){auto c=api.class_from_name(api.assembly_get_image(a[i]),ns,name);if(c)return c;}return nullptr;}
static const void* method(void* c,const char* name,int args){for(;c;c=api.class_get_parent(c)){auto m=api.class_get_method_from_name(c,name,args);if(m)return m;}return nullptr;}
static const void* field(void* c,const char* name){for(;c;c=api.class_get_parent(c)){auto f=api.class_get_field_from_name(c,name);if(f)return f;}return nullptr;}
static void* invoke(const void* m,void* obj,void** args=nullptr){if(!m)return nullptr;void* ex=nullptr;auto r=api.runtime_invoke(m,obj,args,&ex);if(ex){failed=true;log("STOP: managed exception.\r\n");return nullptr;}return r;}
static void* tryInvoke(const void* m,void* obj,void** args=nullptr){if(!m)return nullptr;void* ex=nullptr;auto r=api.runtime_invoke(m,obj,args,&ex);return ex?nullptr:r;}
static bool boolean(void* boxed){auto p=boxed?api.object_unbox(boxed):nullptr;return p&&*static_cast<bool*>(p);}
static bool alive(void* obj){void* a[]={obj};return obj&&boolean(invoke(aliveMethod,nullptr,a));}
static void hold(void* obj){if(obj)holds.push_back(api.gchandle_new(obj,false));}
static void* cls(void* o){return o?api.object_get_class(o):nullptr;}
static void* call(void* o,const char* name,int n=0,void** args=nullptr){auto m=o?method(cls(o),name,n):nullptr;if(!m){failed=true;log("STOP: required method missing.\r\n");return nullptr;}return invoke(m,o,args);}
static const void* typed(void* c,const char* name,int n,unsigned index,const char* typeName){for(;c;c=api.class_get_parent(c)){void* it=nullptr;while(auto m=api.class_get_methods(c,&it)){if(strcmp(api.method_get_name(m),name)||api.method_get_param_count(m)!=(unsigned)n)continue;auto text=api.type_get_name(api.method_get_param(m,index));bool match=text&&!strcmp(text,typeName);if(text)api.free(text);if(match)return m;}}return nullptr;}
static void* type(void* c){return c?api.type_get_object(api.class_get_type(c)):nullptr;}
static void* ref(void* o,const char* name){void* v=nullptr;auto f=o?field(cls(o),name):nullptr;if(f)api.field_get_value(o,f,&v);return v;}
static void* refCached(void* o,const void* f){void* v=nullptr;if(o&&f)api.field_get_value(o,f,&v);return v;}
template<class T> static T value(void* o,const char* name){T v{};auto f=o?field(cls(o),name):nullptr;if(!f){failed=true;return v;}api.field_get_value(o,f,&v);return v;}
template<class T> static T valueCached(void* o,const void* f){T v{};if(o&&f)api.field_get_value(o,f,&v);return v;}
template<class T> static void setField(void* o,const char* name,const T& v){auto f=o?field(cls(o),name):nullptr;if(!f){failed=true;return;}api.field_set_value(o,f,(void*)&v);}
template<class T> static void setFieldCached(void* o,const void* f,const T& v){if(o&&f)api.field_set_value(o,f,(void*)&v);}
template<class T> static void set(void* o,const char* name,T v){void* a[]={&v};call(o,name,1,a);}
static void setref(void* o,const char* name,void* v){void* a[]={v};call(o,name,1,a);}
static void* go(void* c){return invoke(gameObjectMethod,c);} static void* transform(void* g){return call(g,"get_transform");}
static void active(void* g,bool on){set(g,"SetActive",on);}
static void* get(void* g,const char* ns,const char* name){auto c=klass(ns,name);auto m=g?typed(cls(g),"GetComponent",1,0,"System.Type"):nullptr;if(!c||!m)return nullptr;void* a[]={type(c)};return invoke(m,g,a);}
static std::vector<void*> children(void* component,const char* ns,const char* name){std::vector<void*> result;auto c=klass(ns,name);auto owner=go(component);auto m=owner?typed(cls(owner),"GetComponentsInChildren",2,0,"System.Type"):nullptr;if(!c||!m){failed=true;return result;}bool yes=true;void* a[]={type(c),&yes};auto arr=invoke(m,owner,a);if(!arr)return result;auto h=api.gchandle_new(arr,false);auto n=api.array_length(arr);if(n<4096){auto p=il2cpp_array_data<void*>(arr);for(uintptr_t i=0;i<n;i++)if(alive(p[i]))result.push_back(p[i]);}api.gchandle_free(h);return result;}
template<class T> static T boxed(void* value,T fallback={}){auto p=value?api.object_unbox(value):nullptr;return p?*static_cast<T*>(p):fallback;}
static void layoutRelativeToDonor(void* target,void* source,V2 position){
    auto dst=transform(target),src=transform(source);
    set(dst,"set_anchorMin",V2{.5f,.5f});
    set(dst,"set_anchorMax",V2{.5f,.5f});
    set(dst,"set_pivot",boxed<V2>(call(src,"get_pivot"),{.5f,.5f}));
    set(dst,"set_sizeDelta",boxed<V2>(call(src,"get_sizeDelta"),{46,46}));
    set(dst,"set_anchoredPosition",position);
    set(dst,"set_localScale",V3{1,1,1});
}
static void layoutAtLocalPosition(void* target,void* source,V3 position){auto dst=transform(target),src=transform(source);set(dst,"set_pivot",boxed<V2>(call(src,"get_pivot"),{.5f,.5f}));set(dst,"set_sizeDelta",boxed<V2>(call(src,"get_sizeDelta"),{46,46}));set(dst,"set_localPosition",position);set(dst,"set_localScale",V3{1,1,1});}
static V3 transformPoint(void* t,V3 point){void* args[]={&point};return boxed<V3>(call(t,"TransformPoint",1,args));}
static V3 inverseTransformPoint(void* t,V3 point){void* args[]={&point};return boxed<V3>(call(t,"InverseTransformPoint",1,args));}
static void syncToolbarOverlay(){
    bool visible=alive(toolbarDonorObject)&&boolean(invoke(activeMethod,toolbarDonorObject));
    if(visible!=toolbarVisible){toolbarVisible=visible;for(auto b:buttons)if(alive(b))active(b,visible);}
    if(!visible||!toolbarDonorTransform||!toolbarOverlayParent)return;
    auto world=boxed<V3>(call(toolbarDonorTransform,"get_position"));
    if(toolbarPositionValid&&fabsf(world.x-lastToolbarWorldPosition.x)<.01f&&fabsf(world.y-lastToolbarWorldPosition.y)<.01f&&fabsf(world.z-lastToolbarWorldPosition.z)<.01f)return;
    toolbarPositionValid=true;lastToolbarWorldPosition=world;constexpr float step=-84.f;
    for(int i=0;i<3;i++)if(alive(buttons[i])){auto desiredWorld=transformPoint(toolbarDonorTransform,V3{0,step*(i+1),0});auto desiredLocal=inverseTransformPoint(toolbarOverlayParent,desiredWorld);set(transform(buttons[i]),"set_localPosition",desiredLocal);}
}
static void fitIcon(void* graphic){
    // Keep the cloned native icon RectTransform intact: its anchors, size and
    // centre already match the stock toolbar. Preserve the replacement
    // sprite's aspect ratio inside that known-good frame.
    if(graphic)set(graphic,"set_preserveAspect",true);
}
static void tooltipText(void* tip,const char* name,bool on){
    if(!tip)return;char text[180];snprintf(text,sizeof(text),"%s: %s",name,on?"ON":"OFF");
    TooltipText value{api.string_new(text),0,0};auto f=field(cls(tip),"_tooltip");if(f)api.field_set_value(tip,f,&value);call(tip,"Reset");
}
static void useClonedTooltip(int index,void* button,const char* name){
    auto found=children(button,"TPS.Game.UI","TooltipSpawner");if(found.empty()){log("UI: cloned tooltip unavailable.\r\n");return;}tips[index]=found[0];tooltipText(tips[index],name,flags[index]);
}
static void* cloneDetachedObject(void* original){
    // Instantiate with no parent. Parenting a clone into the stock layout,
    // even briefly, makes its shared backing image expand and leaves the dark
    // translucent strips visible in other hub menus.
    auto instantiate=typed(klass("UnityEngine","Object"),"Instantiate",1,0,"UnityEngine.Object");void* args[]={original};return invoke(instantiate,nullptr,args);
}
static void* firstObjectOfType(void* wantedType){
    if(!wantedType)return nullptr;void* args[]={wantedType};auto arr=invoke(findAll,nullptr,args);if(!arr)return nullptr;auto handle=api.gchandle_new(arr,false);void* result=nullptr;auto count=api.array_length(arr);if(count<128){auto entries=il2cpp_array_data<void*>(arr);for(uintptr_t i=0;i<count;i++)if(alive(entries[i])){result=entries[i];break;}}api.gchandle_free(handle);return result;
}
static void attachDemolitionButtons(void* menu,void* itemsDonorTransform){
    auto foundations=firstObjectOfType(foundationsType);if(!foundations){log("DEMOLITION: Foundations menu unavailable; Items buttons not attached.\r\n");return;}demolitionTarget=foundations;hold(foundations);void* sources[]={ref(foundations,"_demolishToolsDragSelectDemolish"),ref(foundations,"_demolishToolsNuke")};constexpr V2 offsets[]={{-13.f,-543.f},{-13.f,-627.f}};
    auto closeRoot=ref(menu,"_rootCloseButton");constexpr V3 closeOffsets[]={{44.f,-1282.f,0},{44.f,-1366.f,0}};auto parent=closeRoot?closeRoot:itemsDonorTransform;
    for(int i=0;i<2;i++){auto source=sources[i],sourceObject=source?go(source):nullptr;if(!sourceObject){log("DEMOLITION: native source button unavailable.\r\n");return;}auto clone=cloneDetachedObject(sourceObject);if(!clone){log("DEMOLITION: native button clone failed.\r\n");return;}demolitionObjects[i]=clone;hold(clone);active(clone,false);auto cloneTransform=transform(clone);bool world=false;void* parentArgs[]={parent,&world};call(cloneTransform,"SetParent",2,parentArgs);if(closeRoot)layoutAtLocalPosition(clone,sourceObject,closeOffsets[i]);else layoutRelativeToDonor(clone,sourceObject,offsets[i]);call(cloneTransform,"SetAsLastSibling");auto button=get(clone,"TPS.Core.UI","UIButton");if(!button){log("DEMOLITION: cloned UIButton component missing.\r\n");return;}if(!mouseDownField)mouseDownField=field(cls(button),"_mouseDownLeft");auto event=ref(button,"onClick");if(event)call(event,"RemoveAllListeners");call(button,"ClearAllNavigation");set(button,"set_interactable",true);demolitionButtons[i]=button;previousDemolitionDown[i]=false;active(clone,true);log(closeRoot?"DEMOLITION: button anchored to always-visible Items root.\r\n":"DEMOLITION: close root missing; button anchored to Grid.\r\n");}log("DEMOLITION: two independent native buttons attached to Items.\r\n");
}
static void suppressImage(void* image,const char* reason){if(!alive(image))return;set(image,"set_enabled",false);set(image,"set_raycastTarget",false);auto color=boxed<Color>(call(image,"get_color"),Color{0,0,0,0});color.a=0;set(image,"set_color",color);static int logged=0;if(logged<8){log(reason);logged++;}}
static bool belongsToButton(void* imageTransform,void* panelTransform){auto current=imageTransform;for(int depth=0;current&&depth<10;depth++){if(get(go(current),"TPS.Core.UI","UIButton"))return true;if(current==panelTransform)break;current=call(current,"get_parent");}return false;}
static void suppressPlacementBackground(void* menu){auto direct=ref(menu,"_placementToolsBg");if(alive(direct))suppressImage(direct,"BACKDROP: serialized placement background suppressed.\r\n");auto donor=ref(menu,"_placementToolsToggleGrid");if(!donor)donor=ref(menu,"_buttonLayoutGroup");auto current=donor?call(transform(go(donor)),"get_parent"):nullptr;auto menuTransform=transform(go(menu));for(int depth=0;current&&current!=menuTransform&&depth<6;depth++){auto owner=go(current),image=get(owner,"UnityEngine.UI","Image");if(alive(image)){auto rect=boxed<Rect>(call(current,"get_rect"));if(fabsf(rect.w)<=220.f)suppressImage(image,"BACKDROP: narrow toolbar ancestor image suppressed.\r\n");}current=call(current,"get_parent");}auto panel=ref(menu,"_placementToolsPanel");auto panelTransform=panel?transform(panel):nullptr;if(panelTransform){auto images=children(panelTransform,"UnityEngine.UI","Image");for(auto image:images){auto imageTransform=transform(go(image));if(!belongsToButton(imageTransform,panelTransform))suppressImage(image,"BACKDROP: non-button placement-panel image suppressed.\r\n");}}}
static void suppressPlacementBackgrounds(void* wantedType){if(!wantedType)return;void* args[]={wantedType};auto arr=invoke(findAll,nullptr,args);if(!arr)return;auto handle=api.gchandle_new(arr,false);auto count=api.array_length(arr);if(count<128){auto entries=il2cpp_array_data<void*>(arr);for(uintptr_t i=0;i<count;i++)if(alive(entries[i]))suppressPlacementBackground(entries[i]);}api.gchandle_free(handle);}
static void replaceStateIcons(void* button,void* offSprite,void* onSprite){
    auto states=ref(button,"_stateData");if(!states)return;for(auto stateName:{"Normal","Locked","CallToAction","Negative"}){auto data=ref(states,stateName);auto icon=data?field(cls(data),"ChildIcon"):nullptr;if(icon)api.field_set_value_object(data,icon,offSprite);}for(auto stateName:{"Selected","NegativeSelected"}){auto data=ref(states,stateName);auto icon=data?field(cls(data),"ChildIcon"):nullptr;if(icon)api.field_set_value_object(data,icon,onSprite);}
}

static Color32 hsvPixel(float h,float s=.8f,float v=1.f){auto c=random_logic::hsv_to_rgb({h,s,v},1);return {(unsigned char)(c.r*255),(unsigned char)(c.g*255),(unsigned char)(c.b*255),255};}
static void pixel(std::vector<Color32>& p,int x,int y,Color32 c){if(x>=0&&x<32&&y>=0&&y<32)p[(size_t)y*32+x]=c;}
static void disc(std::vector<Color32>& p,int cx,int cy,int r,Color32 c){for(int y=-r;y<=r;y++)for(int x=-r;x<=r;x++)if(x*x+y*y<=r*r)pixel(p,cx+x,cy+y,c);}
static void line(std::vector<Color32>& p,int x0,int y0,int x1,int y1,int thick,Color32 c){int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,e=dx+dy;for(;;){disc(p,x0,y0,thick,c);if(x0==x1&&y0==y1)break;int e2=2*e;if(e2>=dy){e+=dy;x0+=sx;}if(e2<=dx){e+=dx;y0+=sy;}}}
static bool sameString(void* managed,const wchar_t* expected){if(!managed||!expected)return false;auto n=api.string_length(managed);auto wanted=wcslen(expected);return n>=0&&(size_t)n==wanted&&!wmemcmp(api.string_chars(managed),expected,wanted);}
static bool containsString(void* managed,const wchar_t* expected){if(!managed||!expected)return false;auto n=api.string_length(managed);auto wanted=wcslen(expected);if(n<0||(size_t)n<wanted)return false;auto text=api.string_chars(managed);for(size_t i=0;i+wanted<=(size_t)n;i++)if(!wmemcmp(text+i,expected,wanted))return true;return false;}
static void whitenPixels(void* pixels,int w,int h){if(!pixels||w<=0||h<=0)return;auto count=api.array_length(pixels);if(count!=(uintptr_t)w*(uintptr_t)h)return;auto data=il2cpp_array_data<Color32>(pixels);for(uintptr_t i=0;i<count;i++)data[i].r=data[i].g=data[i].b=255;}
static void* spriteFromPixels(void* pixels,int w,int h){
    if(!pixels||w<=0||h<=0)return nullptr;auto pixelsHandle=api.gchandle_new(pixels,false);auto texture=api.object_new(klass("UnityEngine","Texture2D"));hold(texture);void* ctorArgs[]={&w,&h};call(texture,".ctor",2,ctorArgs);void* pixelArgs[]={pixels};call(texture,"SetPixels32",1,pixelArgs);call(texture,"Apply");Rect bounds{0,0,(float)w,(float)h};V2 pivot{.5f,.5f};void* createArgs[]={texture,&bounds,&pivot};auto sprite=invoke(typed(klass("UnityEngine","Sprite"),"Create",3,0,"UnityEngine.Texture2D"),nullptr,createArgs);hold(sprite);api.gchandle_free(pixelsHandle);return sprite;
}
static void* whiteSpriteFromTexture(void* source,int w,int h){
    if(!source||w<=0||h<=0)return nullptr;auto getPixels=method(cls(source),"GetPixels32",0);auto pixels=tryInvoke(getPixels,source);if(pixels&&api.array_length(pixels)==(uintptr_t)w*(uintptr_t)h){whitenPixels(pixels,w,h);log("ICONS: rotation texture normalized directly.\r\n");return spriteFromPixels(pixels,w,h);}
    auto renderTextureClass=klass("UnityEngine","RenderTexture"),graphicsClass=klass("UnityEngine","Graphics");if(!renderTextureClass||!graphicsClass)return nullptr;auto renderTexture=api.object_new(renderTextureClass);auto renderHandle=api.gchandle_new(renderTexture,false);int depth=0;void* ctorArgs[]={&w,&h,&depth};invoke(method(renderTextureClass,".ctor",3),renderTexture,ctorArgs);auto blit=typed(graphicsClass,"Blit",2,1,"UnityEngine.RenderTexture");void* blitArgs[]={source,renderTexture};invoke(blit,nullptr,blitArgs);auto getActive=method(renderTextureClass,"get_active",0),setActive=method(renderTextureClass,"set_active",1);auto previous=invoke(getActive,nullptr);void* activeArgs[]={renderTexture};invoke(setActive,nullptr,activeArgs);auto readable=api.object_new(klass("UnityEngine","Texture2D"));hold(readable);void* readableCtorArgs[]={&w,&h};call(readable,".ctor",2,readableCtorArgs);Rect rect{0,0,(float)w,(float)h};int x=0,y=0;void* readArgs[]={&rect,&x,&y};call(readable,"ReadPixels",3,readArgs);call(readable,"Apply");void* restoreArgs[]={previous};invoke(setActive,nullptr,restoreArgs);call(renderTexture,"Release");auto destroy=typed(klass("UnityEngine","Object"),"Destroy",1,0,"UnityEngine.Object");void* destroyArgs[]={renderTexture};invoke(destroy,nullptr,destroyArgs);api.gchandle_free(renderHandle);pixels=call(readable,"GetPixels32");if(!pixels||api.array_length(pixels)!=(uintptr_t)w*(uintptr_t)h)return nullptr;whitenPixels(pixels,w,h);void* pixelArgs[]={pixels};call(readable,"SetPixels32",1,pixelArgs);call(readable,"Apply");Rect bounds{0,0,(float)w,(float)h};V2 pivot{.5f,.5f};void* createArgs[]={readable,&bounds,&pivot};auto sprite=invoke(typed(klass("UnityEngine","Sprite"),"Create",3,0,"UnityEngine.Texture2D"),nullptr,createArgs);hold(sprite);log("ICONS: rotation texture normalized through GPU readback.\r\n");return sprite;
}
static void resolveGameIcons(){
    if(gameIconsResolved)return;gameIconsResolved=true;const wchar_t* wanted[]={L"T_UI_RoomItemRotation",L"UI_Generic_T_Spritesheet_Building_itemCustomise",L"UI_Atlas_Icons_Inspectors_TabKnowledge_White"};auto spriteClass=klass("UnityEngine","Sprite");auto spriteType=type(spriteClass);if(!spriteClass||!spriteType||!objectNameMethod){log("ICONS: game sprite lookup unavailable; using fallback icons.\r\n");return;}auto typeHandle=api.gchandle_new(spriteType,false);void* args[]={spriteType};auto arr=invoke(findAll,nullptr,args);if(arr){auto arrayHandle=api.gchandle_new(arr,false);auto count=api.array_length(arr);if(count<65536){auto entries=il2cpp_array_data<void*>(arr);for(uintptr_t i=0;i<count;i++){auto candidate=entries[i];if(!candidate)continue;auto name=invoke(objectNameMethod,candidate);for(int k=0;k<3;k++)if(!gameIcons[k]&&(sameString(name,wanted[k])||(k==0&&containsString(name,L"RoomItemRotation")))){gameIcons[k]=candidate;gameIconHandles[k]=api.gchandle_new(candidate,false);}if(gameIcons[0]&&gameIcons[1]&&gameIcons[2])break;}}api.gchandle_free(arrayHandle);}api.gchandle_free(typeHandle);
    // The requested rotation asset can be imported as a Texture2D rather than
    // an individually named Sprite. In that case, wrap the full texture in a
    // centred sprite so the exact in-game artwork is still used.
    if(!gameIcons[0]){auto textureClass=klass("UnityEngine","Texture2D"),textureType=type(textureClass);if(textureClass&&textureType){auto textureTypeHandle=api.gchandle_new(textureType,false);void* textureArgs[]={textureType};auto textures=invoke(findAll,nullptr,textureArgs);if(textures){auto texturesHandle=api.gchandle_new(textures,false);auto count=api.array_length(textures);if(count<65536){auto entries=il2cpp_array_data<void*>(textures);for(uintptr_t i=0;i<count;i++){auto texture=entries[i];if(!texture)continue;auto name=invoke(objectNameMethod,texture);if(!sameString(name,wanted[0])&&!containsString(name,L"RoomItemRotation"))continue;int w=boxed<int>(call(texture,"get_width")),h=boxed<int>(call(texture,"get_height"));auto created=whiteSpriteFromTexture(texture,w,h);if(created){gameIcons[0]=created;gameIconHandles[0]=api.gchandle_new(created,false);log("ICONS: rotation artwork resolved from its normalized game texture.\r\n");}break;}}api.gchandle_free(texturesHandle);}api.gchandle_free(textureTypeHandle);}}
    log(gameIcons[0]&&gameIcons[1]&&gameIcons[2]?"ICONS: all requested game artwork resolved.\r\n":"ICONS: one or more requested assets missing; using fallback where needed.\r\n");
}
static void* generatedIconSprite(int kind){std::vector<Color32> px(32*32,{0,0,0,0});Color32 ink{255,247,213,255},dark{57,96,114,255};
    if(kind==0){for(int d=25;d<315;d+=5){float a=d*3.14159265f/180.f;disc(px,16+(int)(10*cosf(a)),16+(int)(10*sinf(a)),1,ink);}line(px,7,8,7,15,1,ink);line(px,7,8,14,8,1,ink);disc(px,16,16,2,dark);}
    if(kind==1){for(int y=5;y<=27;y++)for(int x=5;x<=27;x++){int dx=x-16,dy=y-16;if(dx*dx+dy*dy<=121){float h=atan2f((float)dy,(float)dx)/6.2831853f+.5f;pixel(px,x,y,hsvPixel(h));}}disc(px,16,16,3,ink);}
    if(kind==2){disc(px,16,16,7,ink);for(int y=9;y<=23;y++)for(int x=9;x<16;x++)pixel(px,x,y,dark);for(int i=0;i<8;i++){float a=i*3.14159265f/4;line(px,16+(int)(10*cosf(a)),16+(int)(10*sinf(a)),16+(int)(14*cosf(a)),16+(int)(14*sinf(a)),1,ink);}}
    auto arr=api.array_new(klass("UnityEngine","Color32"),px.size());hold(arr);memcpy(il2cpp_array_data<Color32>(arr),px.data(),px.size()*sizeof(Color32));
    auto tex=api.object_new(klass("UnityEngine","Texture2D"));hold(tex);int w=32,h=32;void* ca[]={&w,&h};call(tex,".ctor",2,ca);void* sa[]={arr};call(tex,"SetPixels32",1,sa);call(tex,"Apply");set(tex,"set_filterMode",1);
    Rect bounds{0,0,32,32};V2 pivot{.5f,.5f};void* args[]={tex,&bounds,&pivot};auto sprite=invoke(typed(klass("UnityEngine","Sprite"),"Create",3,0,"UnityEngine.Texture2D"),nullptr,args);hold(sprite);return sprite;
}
static void* iconSprite(int kind){resolveGameIcons();return kind>=0&&kind<3&&gameIcons[kind]?gameIcons[kind]:generatedIconSprite(kind);}
static void* readSpritePixels(void* sprite,int& w,int& h){
    w=h=0;if(!sprite)return nullptr;auto texture=tryInvoke(method(cls(sprite),"get_texture",0),sprite);auto region=boxed<Rect>(tryInvoke(method(cls(sprite),"get_textureRect",0),sprite));w=(int)lroundf(region.w);h=(int)lroundf(region.h);if(!texture||w<=0||h<=0)return nullptr;int textureW=boxed<int>(call(texture,"get_width")),textureH=boxed<int>(call(texture,"get_height"));auto all=tryInvoke(method(cls(texture),"GetPixels32",0),texture);if(all&&api.array_length(all)==(uintptr_t)textureW*(uintptr_t)textureH){auto cropped=api.array_new(klass("UnityEngine","Color32"),(uintptr_t)w*(uintptr_t)h);hold(cropped);auto src=il2cpp_array_data<Color32>(all),dst=il2cpp_array_data<Color32>(cropped);int sx=(int)lroundf(region.x),sy=(int)lroundf(region.y);for(int y=0;y<h;y++)memcpy(dst+(size_t)y*w,src+(size_t)(sy+y)*textureW+sx,(size_t)w*sizeof(Color32));return cropped;}
    auto renderTextureClass=klass("UnityEngine","RenderTexture"),graphicsClass=klass("UnityEngine","Graphics");if(!renderTextureClass||!graphicsClass)return nullptr;auto renderTexture=api.object_new(renderTextureClass);auto renderHandle=api.gchandle_new(renderTexture,false);int depth=0;void* ctorArgs[]={&textureW,&textureH,&depth};invoke(method(renderTextureClass,".ctor",3),renderTexture,ctorArgs);auto blit=typed(graphicsClass,"Blit",2,1,"UnityEngine.RenderTexture");void* blitArgs[]={texture,renderTexture};invoke(blit,nullptr,blitArgs);auto getActive=method(renderTextureClass,"get_active",0),setActive=method(renderTextureClass,"set_active",1);auto previous=invoke(getActive,nullptr);void* activeArgs[]={renderTexture};invoke(setActive,nullptr,activeArgs);auto readable=api.object_new(klass("UnityEngine","Texture2D"));hold(readable);void* readableCtorArgs[]={&w,&h};call(readable,".ctor",2,readableCtorArgs);int x=0,y=0;void* readArgs[]={&region,&x,&y};call(readable,"ReadPixels",3,readArgs);call(readable,"Apply");void* restoreArgs[]={previous};invoke(setActive,nullptr,restoreArgs);call(renderTexture,"Release");auto destroy=typed(klass("UnityEngine","Object"),"Destroy",1,0,"UnityEngine.Object");void* destroyArgs[]={renderTexture};invoke(destroy,nullptr,destroyArgs);api.gchandle_free(renderHandle);return call(readable,"GetPixels32");
}
static void* offIconSprite(int kind,void* selectedSprite){
    if(kind<0||kind>=3)return selectedSprite;if(offIcons[kind])return offIcons[kind];int w=0,h=0;auto pixels=readSpritePixels(selectedSprite,w,h);if(!pixels)return selectedSprite;auto count=api.array_length(pixels);auto data=il2cpp_array_data<Color32>(pixels);constexpr Color32 compensated{167,85,15,255};for(uintptr_t i=0;i<count;i++){data[i].r=compensated.r;data[i].g=compensated.g;data[i].b=compensated.b;}auto sprite=spriteFromPixels(pixels,w,h);if(sprite){offIcons[kind]=sprite;offIconHandles[kind]=api.gchandle_new(sprite,false);}return sprite?sprite:selectedSprite;
}

static void destroyUi(){auto destroy=typed(klass("UnityEngine","Object"),"Destroy",1,0,"UnityEngine.Object");for(auto b:buttons)if(alive(b)){void* args[]={b};invoke(destroy,nullptr,args);}for(auto b:demolitionObjects)if(alive(b)){void* args[]={b};invoke(destroy,nullptr,args);}for(auto h:holds)if(h)api.gchandle_free(h);holds.clear();itemsMenu=demolitionTarget=toolbarDonorObject=toolbarDonorTransform=toolbarOverlayParent=nullptr;toolbarVisible=toolbarPositionValid=false;lastToolbarWorldPosition={};for(int i=0;i<3;i++){buttons[i]=nativeButtons[i]=childGraphics[i]=icons[i]=tips[i]=nullptr;previousDown[i]=false;}for(int i=0;i<2;i++){demolitionObjects[i]=demolitionButtons[i]=nullptr;previousDemolitionDown[i]=false;}}
static bool attachUi(void* menu){auto donor=ref(menu,"_placementToolsToggleGrid");if(!donor)return false;auto donorObject=go(donor),donorTransform=transform(donorObject);const char* names[]={"Random rotation","Random colour","Random brightness"};
    V2 donorPosition=boxed<V2>(call(donorTransform,"get_anchoredPosition"));constexpr float step=-84.0f;
    char placement[180];snprintf(placement,sizeof(placement),"UI LAYOUT: donor anchored=(%.1f,%.1f) step=%.1f.\r\n",donorPosition.x,donorPosition.y,step);log(placement);
    // Clone all three pristine donor buttons while fully detached. This both
    // avoids recursive child cloning and keeps the stock layout/background at
    // its original size.
    for(int i=0;i<3&&!failed;i++){buttons[i]=cloneDetachedObject(donorObject);if(!buttons[i]){failed=true;log("UI STOP: native button clone failed.\r\n");break;}hold(buttons[i]);active(buttons[i],false);}
    toolbarDonorObject=donorObject;toolbarDonorTransform=donorTransform;toolbarOverlayParent=transform(go(menu));toolbarVisible=false;toolbarPositionValid=false;
    for(int i=0;i<3&&!failed;i++){auto b=buttons[i],t=transform(b);setref(b,"set_name",api.string_new(names[i]));bool world=false;void* parentArgs[]={toolbarOverlayParent,&world};call(t,"SetParent",2,parentArgs);layoutAtLocalPosition(b,donorObject,V3{});auto native=get(b,"TPS.Core.UI","UIButton");if(!native){failed=true;log("UI STOP: cloned UIButton component missing.\r\n");break;}if(!mouseDownField)mouseDownField=field(cls(native),"_mouseDownLeft");if(!mouseDownField){failed=true;log("UI STOP: button input field missing.\r\n");break;}auto event=ref(native,"onClick");if(event)call(event,"RemoveAllListeners");call(native,"ClearAllNavigation");set(native,"set_interactable",true);auto selectedSprite=iconSprite(i),normalSprite=offIconSprite(i,selectedSprite);replaceStateIcons(native,normalSprite,selectedSprite);auto child=call(native,"get_ChildGraphic");if(child)fitIcon(child);call(t,"SetAsLastSibling");set(native,"SetSelected",flags[i]);if(child)setref(child,"set_sprite",flags[i]?selectedSprite:normalSprite);nativeButtons[i]=native;childGraphics[i]=child;icons[i]=selectedSprite;previousDown[i]=false;useClonedTooltip(i,native,names[i]);char line[140];snprintf(line,sizeof(line),"UI: button %d detached from Grid hover hierarchy.\r\n",i+1);log(line);}
    syncToolbarOverlay();
    if(failed){destroyUi();return false;}suppressPlacementBackground(menu);attachDemolitionButtons(menu,donorTransform);itemsMenu=menu;hold(menu);log("UI: three random-placement toggles attached.\r\n");return true;
}

static void saveFlags(){WritePrivateProfileStringW(L"RandomPlacement",L"Rotation",flags[0]?L"1":L"0",iniPath.c_str());WritePrivateProfileStringW(L"RandomPlacement",L"Colour",flags[1]?L"1":L"0",iniPath.c_str());WritePrivateProfileStringW(L"RandomPlacement",L"Brightness",flags[2]?L"1":L"0",iniPath.c_str());}
static void disableAllOnClose(){bool changed=false;for(int i=0;i<3;i++){changed|=flags[i];flags[i]=false;lastFlags[i]=false;}lastItem=nullptr;if(changed){saveFlags();log("TOGGLES: all options reset off when the Items menu closed.\r\n");}}
static void updateButtons(){const char* names[]={"Random rotation","Random colour","Random brightness"};for(int i=0;i<3;i++){if(!alive(nativeButtons[i]))continue;bool down=valueCached<bool>(nativeButtons[i],mouseDownField);bool pressed=down&&!previousDown[i];previousDown[i]=down;if(!pressed)continue;flags[i]=!flags[i];lastFlags[i]=flags[i];lastItem=nullptr;set(nativeButtons[i],"SetSelected",flags[i]);auto child=childGraphics[i];if(child)setref(child,"set_sprite",flags[i]?icons[i]:offIcons[i]);tooltipText(tips[i],names[i],flags[i]);saveFlags();log(i==0?"TOGGLE: rotation changed.\r\n":i==1?"TOGGLE: colour changed.\r\n":"TOGGLE: brightness changed.\r\n");}}
static void updateDemolitionButtons(){if(!alive(demolitionTarget))return;for(int i=0;i<2;i++){if(!alive(demolitionButtons[i]))continue;bool down=valueCached<bool>(demolitionButtons[i],mouseDownField);bool pressed=down&&!previousDemolitionDown[i];previousDemolitionDown[i]=down;if(!pressed)continue;if(i==0){void* logic=nullptr;if(buildLogicInstanceField)api.field_static_get_value(buildLogicInstanceField,&logic);if(logic){call(logic,"StartGameDragSelectDemolish");log("DEMOLITION: direct drag-select mode started from Items.\r\n");}else log("DEMOLITION: BuildLogic instance unavailable.\r\n");}else{call(demolitionTarget,"OnDemolishToolsNuke");log("DEMOLITION: Demolish Museum selected from Items.\r\n");}}}

static bool getDefaultTint(void* definition,NullableTint& tint){
    auto components=definition?ref(definition,"Components"):nullptr;if(!components||!tintProxyClass)return false;auto handle=api.gchandle_new(components,false);auto count=api.array_length(components);bool found=false;
    if(count<4096){auto entries=il2cpp_array_data<void*>(components);for(uintptr_t i=0;i<count;i++){auto proxy=entries[i];if(!proxy||cls(proxy)!=tintProxyClass)continue;auto result=call(proxy,"GetDefault");auto raw=result?api.object_unbox(result):nullptr;if(raw){tint={true,{0,0,0},*static_cast<Tint*>(raw)};found=true;}break;}}
    api.gchandle_free(handle);if(found)log("TINT: initialized colour data from the item definition.\r\n");return found;
}
static void randomisePlacement(){void* manager=nullptr;if(cursorInstanceField)api.field_static_get_value(cursorInstanceField,&manager);if(!manager)return;auto mode=refCached(manager,cursorModeField);if(!mode||cls(mode)!=placeClass){lastItem=nullptr;return;}auto item=refCached(mode,placeItemField);if(!item||valueCached<int>(mode,placeEditModeField)!=0){lastItem=nullptr;return;}if(item==lastItem)return;lastItem=item;
    auto definition=refCached(mode,placeDefinitionField);
    if(flags[0]){float snap=definition?value<float>(definition,"RotationSnap"):90.f;bool freePlacement=boolean(invoke(isFreePlacementMethod,mode));float angle=random_logic::snapped_rotation(rng,freePlacement,snap);setFieldCached(mode,placeDesiredRotationField,angle);auto rotate=refCached(mode,placeRotateControlField);if(rotate)set(rotate,"set_Rotation",angle);set(item,"set_LocalRotation",angle);}
    if(flags[1]||flags[2]){NullableTint tint=valueCached<NullableTint>(mode,placeTintColorsField);if(tint.hasValue||getDefaultTint(definition,tint)){random_logic::Rgba original{tint.value.color.r,tint.value.color.g,tint.value.color.b,tint.value.color.a};float hue=flags[1]?hueBag.next(rng):-1.0f;auto next=random_logic::randomise_colour(rng,original,flags[1],flags[2],hue);tint.value.color={next.r,next.g,next.b,next.a};setFieldCached(mode,placeTintColorsField,tint);void* args[]={&tint};invoke(method(cls(item),"SetTintColors",1),item,args);}}
}

static bool initialize(){auto lib=GetModuleHandleW(L"GameAssembly.dll");if(!lib)return false;
#define R(name) api.name=reinterpret_cast<decltype(api.name)>(GetProcAddress(lib,"il2cpp_" #name));if(!api.name){failed=true;log("STOP: missing IL2CPP export.\r\n");return false;}
    R(domain_get) R(thread_current) R(domain_get_assemblies) R(assembly_get_image) R(class_from_name) R(class_get_parent)
    R(class_get_method_from_name) R(class_get_field_from_name) R(object_get_class) R(runtime_invoke) R(class_get_type) R(type_get_object)
    R(object_unbox) R(field_get_value) R(field_static_get_value) R(field_set_value) R(field_set_value_object) R(gchandle_new) R(gchandle_free) R(array_length)
    R(object_new) R(array_new) R(string_new) R(string_chars) R(string_length) R(class_get_methods) R(method_get_name) R(method_get_param_count) R(method_get_param) R(type_get_name) R(free)
#undef R
    auto menu=klass("TPS.Game.UI","ItemsMenu"),foundations=klass("TPS.Game.UI","FoundationsMenu"),rooms=klass("TPS.Game.UI","RoomsMenu"),resources=klass("UnityEngine","Resources"),object=klass("UnityEngine","Object"),component=klass("UnityEngine","Component"),gameObject=klass("UnityEngine","GameObject");
    cursorClass=klass("TPS.Game","CursorManager");placeClass=klass("TPS.Game","CursorModePlaceItem");tintProxyClass=klass("TPS.Game","ECPItemTintColors");buildLogicClass=klass("TPS.Game","BuildLogic");
    if(!menu||!foundations||!rooms||!resources||!object||!component||!gameObject||!cursorClass||!placeClass||!tintProxyClass||!buildLogicClass)return false;findAll=method(resources,"FindObjectsOfTypeAll",1);aliveMethod=method(object,"op_Implicit",1);objectNameMethod=method(object,"get_name",0);gameObjectMethod=method(component,"get_gameObject",0);activeMethod=method(gameObject,"get_activeInHierarchy",0);openMethod=method(menu,"get_IsOpeningOrOpen",0);itemsType=type(menu);foundationsType=type(foundations);roomsType=type(rooms);itemsTypeHandle=itemsType?api.gchandle_new(itemsType,false):0;foundationsTypeHandle=foundationsType?api.gchandle_new(foundationsType,false):0;roomsTypeHandle=roomsType?api.gchandle_new(roomsType,false):0;
    cursorInstanceField=field(cursorClass,"<Instance>k__BackingField");cursorModeField=field(cursorClass,"_mode");placeItemField=field(placeClass,"<Item>k__BackingField");placeEditModeField=field(placeClass,"_editMode");placeDefinitionField=field(placeClass,"_definition");placeDesiredRotationField=field(placeClass,"_desiredRotation");placeRotateControlField=field(placeClass,"_rotateControl");placeTintColorsField=field(placeClass,"_tintColors");isFreePlacementMethod=method(placeClass,"get_IsFreePlacement",0);buildLogicInstanceField=field(buildLogicClass,"<Instance>k__BackingField");
    if(!findAll||!aliveMethod||!objectNameMethod||!gameObjectMethod||!activeMethod||!openMethod||!itemsTypeHandle||!foundationsTypeHandle||!roomsTypeHandle||!cursorInstanceField||!cursorModeField||!placeItemField||!placeEditModeField||!placeDefinitionField||!placeDesiredRotationField||!placeRotateControlField||!placeTintColorsField||!isFreePlacementMethod||!buildLogicInstanceField)return false;log("READY: Decorative Randomiser 1.0.\r\n");return true;
}
static void tick(){if(failed||GetCurrentThreadId()!=windowThread)return;static ULONGLONG last=0,started=GetTickCount64();auto now=GetTickCount64();if(now-last<30)return;last=now;if(!initialized){initialized=initialize();if(!initialized&&now-started>120000){failed=true;log("STOP: initialization timeout.\r\n");}return;}
    static ULONGLONG nextBackdropScan=0;if(now>=nextBackdropScan){nextBackdropScan=now+500;suppressPlacementBackgrounds(itemsType);suppressPlacementBackgrounds(roomsType);}
    if(itemsMenu&&(!alive(itemsMenu)||!boolean(invoke(activeMethod,go(itemsMenu)))||!boolean(invoke(openMethod,itemsMenu)))){disableAllOnClose();destroyUi();log("UI: Items menu closed or replaced.\r\n");}
    static ULONGLONG nextMenuScan=0;if(!itemsMenu&&now>=nextMenuScan){nextMenuScan=now+500;void* a[]={itemsType};auto arr=invoke(findAll,nullptr,a);if(arr){auto h=api.gchandle_new(arr,false);auto n=api.array_length(arr);if(n<128){auto p=il2cpp_array_data<void*>(arr);for(uintptr_t i=0;i<n&&!itemsMenu;i++)if(alive(p[i])&&boolean(invoke(activeMethod,go(p[i])))&&boolean(invoke(openMethod,p[i])))attachUi(p[i]);}api.gchandle_free(h);}}
    if(itemsMenu){syncToolbarOverlay();updateButtons();updateDemolitionButtons();}randomisePlacement();
}
static LRESULT CALLBACK onMessage(int code,WPARAM w,LPARAM l){if(code>=0&&w==PM_REMOVE){auto m=reinterpret_cast<MSG*>(l);if(m->message==WM_QUIT)stopping.store(true);if(m->hwnd==window&&m->message==messageId&&!stopping.load())tick();}return CallNextHookEx(nullptr,code,w,l);}
static BOOL CALLBACK findWindow(HWND h,LPARAM){DWORD pid=0;auto thread=GetWindowThreadProcessId(h,&pid);wchar_t name[80]{};if(pid==GetCurrentProcessId()&&GetClassNameW(h,name,80)&&!wcscmp(name,L"UnityWndClass")){window=h;windowThread=thread;return FALSE;}return TRUE;}
static void run(HMODULE self){auto modDir=parent_path(module_path(self));auto logs=modDir+L"Logs";CreateDirectoryW(logs.c_str(),nullptr);auto saves=modDir+L"Saves";CreateDirectoryW(saves.c_str(),nullptr);iniPath=saves+L"\\RandomPlacement.ini";flags[0]=flags[1]=flags[2]=false;memcpy(lastFlags,flags,sizeof(flags));saveFlags();logFile=CreateFileW((logs+L"\\RandomPlacement.log").c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);log("Decorative Randomiser 1.0 starting.\r\n");for(int i=0;i<30&&!window;i++){EnumWindows(findWindow,0);if(!window)Sleep(1000);}if(!window){log("STOP: Unity window not found.\r\n");return;}messageId=RegisterWindowMessageW(L"DecorativeRandomiser.1.0");auto hook=messageId?SetWindowsHookExW(WH_GETMESSAGE,onMessage,nullptr,windowThread):nullptr;if(!hook){log("STOP: message hook failed.\r\n");return;}while(IsWindow(window)&&!stopping.load()){PostMessageW(window,messageId,0,0);Sleep(20);}stopping.store(true);UnhookWindowsHookEx(hook);log("Decorative Randomiser stopped.\r\n");if(logFile!=INVALID_HANDLE_VALUE)FlushFileBuffers(logFile);}
}

extern "C" __declspec(dllexport) const char* __cdecl DecorativeRandomiser_Version(){return "1.0";}
BOOL APIENTRY DllMain(HMODULE module,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){DisableThreadLibraryCalls(module);if(auto thread=CreateThread(nullptr,0,[](LPVOID p)->DWORD{mod::run((HMODULE)p);return 0;},module,0,nullptr))CloseHandle(thread);}else if(reason==DLL_PROCESS_DETACH)mod::stopping.store(true);return TRUE;}
