/* SellCar 0.2 - adds a "SELL CAR" entry next to "Remove car" in the garage car menu.
   Selling pays a percentage of the model's price plus a percentage of what was paid for the car's upgrades,
   then removes the car exactly like "Remove car" does. "Remove car" itself is untouched (no credits).

   How it works (game build 1.0.0.64, all in memory):
   - the popup menu that lists the car's actions is built by one function (RVA_BUILD); while it runs, the list's
     AddItem is watched and right after the "Remove car" entry (tag 1) a second entry (tag 5, "SELL CAR") is added;
   - choosing tag 5 builds the same kind of popup once more, but with only two entries, "SELL CAR" (tag 6) and
     "BACK" (tag 7), and a title that shows the price (these entries can also be clicked with the mouse);
   - tag 6 adds the credits, then runs the game's own car removal; tag 7 just closes. */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "fh2mod.h"
#include "sc_text.h"

#define RVA_BUILD        0x00CCF600u   /* builds the car's action popup */
#define RVA_ITEM_NEW     0x00CBE780u   /* creates a menu item: (out, int *tag, text holder) */
#define RVA_LOC_NEW      0x0123A570u   /* creates a localized text reference: (out, table, id) */
#define RVA_LOC_TABLE    0x01E48598u   /* the table argument the game passes */
#define RVA_LIST_VTABLE  0x02D60AA0u   /* vtable of that popup's list; slot 8 = AddItem(list, type, item) */
#define RVA_HANDLER      0x00CCC370u   /* the popup's item chosen handler: (page, tag, context) */
#define RVA_REMOVE_CB    0x00CDABC0u   /* confirmation callback of Remove car: (callback object) removes the car */
#define RVA_ITEM_FREE    0x00257730u   /* destroys an item whose reference count reached zero */
#define RVA_PROFILE      0x00CED1B0u   /* returns the object that holds the credits */
#define RVA_GET_CREDITS  0x002DC820u
#define RVA_SET_CREDITS  0x002F5EF0u
#define RVA_LOC_LOOKUP   0x0123D780u   /* (manager, table, key) -> UTF-16 text */
#define RVA_GUID_GARAGE  0x01DFFD00u   /* argument of the context's "garage id" query */
#define RVA_DB_GLOBAL    0x034B3640u
#define RVA_KEY_ACTION   0x0354BD8Cu   /* text id of the popup title ("Select an action") */
#define SELL_TAG         5             /* entry in the car's action menu */
#define CONFIRM_SELL_TAG 6             /* entries of the sell popup */
#define CONFIRM_BACK_TAG 7
#define KEY_ITEM         ((0x7E57u<<16)|0xA600u)
#define KEY_BACK         ((0x7E57u<<16)|0xA601u)
#define MAX_CREDITS      999999999L

static const FH2ModApi *api;
static FH2Mod *mod;
static unsigned char *game;
static char ini_path[MAX_PATH+8];
static int base_pct=50, up_pct=30, reason=7;

typedef void *(*Fn4)(void*,void*,void*,void*);
typedef void **(*ItemNewFn)(void**,int*,void**);
typedef void **(*LocNewFn)(void**,const void*,unsigned);
typedef void *(*AddItemFn)(void*,int,void*);
typedef const uint16_t *(*LookupFn)(void*,unsigned,unsigned);
typedef void *(*DbQueryFn)(void*,void**,const char*);
static Fn4 real_build,real_handler;
static ItemNewFn real_item_new;
static AddItemFn real_add_item;
static LookupFn real_lookup;

static volatile LONG in_builder,added_this_menu,last_tag=-1;
static volatile LONG want_confirm_build;     /* the next build is the sell popup */
static volatile LONG confirm_active;         /* the popup being built / shown is the sell popup */
static volatile LONG sell_gid=-1;
static volatile LONG sell_value;
static uint16_t t_item[24],t_back[24],t_title[200];

static void say(const char *t){ if(api&&mod)api->log(mod,t); }
static void say_n(const char *a,long v,const char *b){
    char m[200];ScBuf s;sc_begin(&s,m,sizeof m);sc_adds(&s,a);sc_addn(&s,v);if(b)sc_adds(&s,b);say(m);
}
static int mapped(const void *p,size_t n){
    MEMORY_BASIC_INFORMATION m;
    if(!p||!n||!VirtualQuery(p,&m,sizeof m))return 0;
    if(m.State!=MEM_COMMIT||(m.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return 0;
    uintptr_t a=(uintptr_t)p,e=(uintptr_t)m.BaseAddress+m.RegionSize;
    return a<=UINTPTR_MAX-n&&a+n<=e;
}
static int code_is(unsigned rva,const unsigned char *want,size_t n){
    if(!mapped(game+rva,n))return 0;
    for(size_t i=0;i<n;i++)if(game[rva+i]!=want[i])return 0;
    return 1;
}

/* ---- the price, from the game's own database ---- */
typedef struct { union{char buf[16];char *ptr;} u; size_t size,cap; } ScStd;   /* MSVC std::string */
static int db_read_int(const char *sql,const char *col,long *out){
    void *const *gp=(void*const*)(game+RVA_DB_GLOBAL);
    if(!mapped(gp,8)||!*gp||!mapped(*gp,8))return 0;
    void *db=*gp;
    void **vt=*(void***)db;
    if(!mapped(vt,9*sizeof(void*))||!vt[8])return 0;
    void *res=NULL;
    ((DbQueryFn)vt[8])(db,&res,sql);
    if(!res||!mapped(res,8))return 0;
    void **rv=*(void***)res;
    int ok=0;
    if(mapped(rv,0x150)){
        if(((int(*)(void*))rv[0x148/8])(res)==1){
            ScStd s;size_t n=0;
            for(int i=0;i<16;i++)s.u.buf[i]=0;
            while(col[n]&&n<15){s.u.buf[n]=col[n];n++;}
            s.size=n;s.cap=15;
            *out=((int(*)(void*,void*))rv[0x110/8])(res,&s);
            ok=1;
        }
        ((void(*)(void*))rv[3])(res);
    }
    return ok;
}
static int price_of(long garage_id,long *value){
    char sql[700];
    sc_price_sql(sql,sizeof sql,garage_id,base_pct,up_pct);
    return db_read_int(sql,"V",value);
}

/* ---- credits ---- */
static int give_credits(long amount){
    void *holder=((void*(*)(void))(game+RVA_PROFILE))();
    if(!holder)return 0;
    long cur=((int(*)(void*))(game+RVA_GET_CREDITS))(holder);
    if(cur<0||cur>MAX_CREDITS){say_n("credits read as ",cur,": not changing anything");return 0;}
    long nv=cur+amount;if(nv>MAX_CREDITS)nv=MAX_CREDITS;
    unsigned char why[0x18];
    for(int i=0;i<0x18;i++)why[i]=0;
    *(int*)(why+4)=-1;*(int*)(why+8)=-1;*(int*)(why+0xC)=-1;*(int*)(why+0x14)=reason;
    ((void(*)(void*,int,void*))(game+RVA_SET_CREDITS))(holder,(int)nv,why);
    long after=((int(*)(void*))(game+RVA_GET_CREDITS))(holder);
    char m[160];ScBuf s;sc_begin(&s,m,sizeof m);
    sc_adds(&s,"sold a car: credits ");sc_addn(&s,cur);sc_adds(&s," -> ");sc_addn(&s,after);sc_adds(&s," (+");sc_addn(&s,amount);sc_adds(&s,")");
    say(m);
    return 1;
}

/* ---- menu entries ---- */
static void add_entry(void *list,int type,int tag,unsigned key){
    void *text=NULL,*item_holder=NULL;
    void **t=((LocNewFn)(game+RVA_LOC_NEW))(&text,game+RVA_LOC_TABLE,key);
    void **i=real_item_new(&item_holder,&tag,t);
    if(i&&*i)real_add_item(list,type,*i);
    if(item_holder){
        LONG *rc=(LONG*)((char*)item_holder+0x10);
        if(InterlockedDecrement(rc)==0)((void(*)(void*))(game+RVA_ITEM_FREE))(item_holder);
    }
    if(text){void **tv=*(void***)text;(*(void(**)(void*))(tv+3))(text);}
}
static void *build_hook(void *a,void *b,void *c,void *d){
    confirm_active=want_confirm_build;want_confirm_build=0;
    in_builder=1;added_this_menu=0;last_tag=-1;
    void *r=real_build(a,b,c,d);
    in_builder=0;
    return r;
}
static void **item_new_hook(void **out,int *tag,void **text){
    if(in_builder&&tag&&mapped(tag,4))last_tag=*tag;
    return real_item_new(out,tag,text);
}
static void *add_item_hook(void *list,int type,void *item){
    if(in_builder&&confirm_active){
        /* the sell popup: drop the game's entries, put ours in at the first call */
        if(!added_this_menu){
            added_this_menu=1;
            add_entry(list,type,CONFIRM_SELL_TAG,KEY_ITEM);
            add_entry(list,type,CONFIRM_BACK_TAG,KEY_BACK);
        }
        return NULL;
    }
    void *r=real_add_item(list,type,item);
    if(in_builder&&last_tag==1&&!added_this_menu){
        added_this_menu=1;
        add_entry(list,type,SELL_TAG,KEY_ITEM);
        static volatile LONG logs=5;
        if(InterlockedDecrement(&logs)>=0)say("SELL CAR entry added to the car action menu");
    }
    return r;
}

/* ---- choosing ---- */
static long garage_id_of(void *ctx){
    if(ctx&&mapped(ctx,8)){
        void **cv=*(void***)ctx;
        if(mapped(cv,8*sizeof(void*)))return ((int(*)(void*,const void*))cv[7])(ctx,game+RVA_GUID_GARAGE);
    }
    return -1;
}
static void do_sale(void *page,void *ctx){
    long gid=garage_id_of(ctx);
    if(gid!=sell_gid||sell_value<0){say("sell: the chosen car is not the one that was priced; nothing done");return;}
    if(sell_value>0&&!give_credits(sell_value)){say("sell: could not add the credits; the car was NOT removed");return;}
    /* the game's own removal, called with the same object its confirmation would have passed */
    uintptr_t cb[5]={0,(uintptr_t)page,(uintptr_t)ctx,0,0};
    *(int*)((char*)cb+0x18)=(int)gid;
    ((void(*)(void*))(game+RVA_REMOVE_CB))(cb);
}
static void *handler_hook(void *page,void *tag_,void *ctx,void *d){
    int tag=(int)(uintptr_t)tag_;
    if(tag==SELL_TAG){
        long gid=garage_id_of(ctx),value=0;
        if(gid<0||!price_of(gid,&value)||value<0){say("sell: could not read the price, nothing done");return NULL;}
        char t[200];ScBuf b;sc_begin(&b,t,sizeof t);
        sc_adds(&b,"SELL THIS CAR FOR ");sc_addn_grouped(&b,value);sc_adds(&b," CR?");
        sc_widen(t_title,sizeof t_title/2,t);
        sell_gid=(LONG)gid;sell_value=(LONG)value;
        say_n("sell: car in garage slot ",gid,NULL);say_n("sell: price ",value," CR");
        want_confirm_build=1;
        return build_hook(page,ctx,NULL,NULL);          /* the sell popup */
    }
    if(tag==CONFIRM_SELL_TAG){confirm_active=0;do_sale(page,ctx);return NULL;}
    if(tag==CONFIRM_BACK_TAG){confirm_active=0;return NULL;}
    return real_handler(page,tag_,ctx,d);
}

/* ---- texts ---- */
static const uint16_t *lookup_hook(void *mgr,unsigned table,unsigned key){
    const uint16_t *r=real_lookup?real_lookup(mgr,table,key):NULL;
    if(key==KEY_ITEM)return t_item;
    if(key==KEY_BACK)return t_back;
    if(confirm_active&&game&&key==*(const unsigned*)(game+RVA_KEY_ACTION))return t_title;
    return r;
}

static DWORD WINAPI install_worker(LPVOID unused){
    (void)unused;
    game=(unsigned char*)api->wait_for_code(0xFFFFFFFFu);
    if(!game){say("the game's code was not ready; nothing installed");return 0;}
    static const unsigned char build_head[]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x8D,0x6C,0x24,0xD9};
    static const unsigned char item_head[]={0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xEC,0x40,0x48,0xC7,0x44,0x24,0x28,0xFE};
    static const unsigned char handler_head[]={0x48,0x8B,0xC4,0x55,0x57,0x41,0x57,0x48,0x8D,0x68,0xA1,0x48,0x81,0xEC,0xF0,0x00};
    static const unsigned char cb_head[]={0x40,0x53,0x48,0x83,0xEC,0x20,0x48,0x8B,0xD9,0xE8,0x42,0x29,0x01,0x00};
    static const unsigned char add_head[]={0x48,0x8B,0xC4,0x57,0x48,0x81,0xEC,0x80,0x00,0x00,0x00,0x48,0xC7,0x40,0x98};
    void **list_vt=(void**)(game+RVA_LIST_VTABLE);
    unsigned char *add_fn=mapped(list_vt+8,8)?(unsigned char*)list_vt[8]:NULL;
    int ok=code_is(RVA_BUILD,build_head,sizeof build_head)&&code_is(RVA_ITEM_NEW,item_head,sizeof item_head)
         &&code_is(RVA_HANDLER,handler_head,sizeof handler_head)&&code_is(RVA_REMOVE_CB,cb_head,sizeof cb_head)
         &&add_fn&&mapped(add_fn,sizeof add_head);
    if(ok)for(unsigned i=0;i<sizeof add_head;i++)if(add_fn[i]!=add_head[i])ok=0;
    if(!ok){say("the game's garage menu code is not the expected build: Sell car stays off (nothing was changed)");return 0;}
    if(!api->hook_function(mod,"menu item list add",add_fn,(void*)add_item_hook,(void**)&real_add_item)||
       !api->hook_function(mod,"menu item create",game+RVA_ITEM_NEW,(void*)item_new_hook,(void**)&real_item_new)||
       !api->hook_function(mod,"car action menu build",game+RVA_BUILD,(void*)build_hook,(void**)&real_build)||
       !api->hook_function(mod,"car action chosen",game+RVA_HANDLER,(void*)handler_hook,(void**)&real_handler)||
       !api->hook_function(mod,"string lookup (Sell car texts)",game+RVA_LOC_LOOKUP,(void*)lookup_hook,(void**)&real_lookup)){
        say("could not set up all hooks: Sell car stays off");
        return 0;
    }
    if(!api->enable_hooks(mod,NULL)){say("could not enable the hooks: Sell car stays off");return 0;}
    say("SELL CAR active: Garage > My Cars > pick a car > the action menu now has SELL CAR after Remove car");
    say_n("price: ",base_pct,"% of the car's price + ");
    say_n("      ",up_pct,"% of what was paid for its upgrades");
    return 0;
}

static void find_ini(void){
    HMODULE hm=NULL;
    ini_path[0]=0;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR)&find_ini,&hm))return;
    DWORD n=GetModuleFileNameA(hm,ini_path,MAX_PATH);
    if(!n||n>=MAX_PATH){ini_path[0]=0;return;}
    int dot=-1;
    for(DWORD i=0;i<n;i++){if(ini_path[i]=='.')dot=(int)i;if(ini_path[i]=='\\'||ini_path[i]=='/')dot=-1;}
    if(dot<0)dot=(int)n;
    const char *ext=".ini";
    for(int i=0;i<5;i++)ini_path[dot+i]=ext[i];
    long b=GetPrivateProfileIntA("Sell","BasePercent",-1,ini_path);
    long u=GetPrivateProfileIntA("Sell","UpgradePercent",-1,ini_path);
    long r=GetPrivateProfileIntA("Sell","CreditReason",-1,ini_path);
    base_pct=sc_clamp_pct(b,50);up_pct=sc_clamp_pct(u,30);
    reason=(r>=0&&r<=30)?(int)r:7;
    if(b<0)WritePrivateProfileStringA("Sell","BasePercent","50",ini_path);
    if(u<0)WritePrivateProfileStringA("Sell","UpgradePercent","30",ini_path);
    if(r<0)WritePrivateProfileStringA("Sell","CreditReason","7",ini_path);
}

FH2MOD_EXPORT int fh2mod_init(const FH2ModApi *platform){
    api=platform;
    if(!api||api->api_version<3||api->size<sizeof(FH2ModApi)||!api->hook_function||!api->enable_hooks||!api->wait_for_code)return 1;
    FH2ModInfo info;
    info.size=sizeof info;info.name="Sell Car";info.version="1.0.0";
    info.github_repo="Levilente/FH2-Sell_cars";info.platform_version="2.8.0";info.check_updates=1;
    info.description="Adds SELL CAR next to Remove car in the garage: pays a percentage of the price and of the upgrades.";
    mod=api->register_mod(&info);
    if(!mod)return 1;
    find_ini();
    sc_widen(t_item,sizeof t_item/2,"SELL CAR");
    sc_widen(t_back,sizeof t_back/2,"BACK");
    sc_widen(t_title,sizeof t_title/2,"SELL THIS CAR?");
    HANDLE t=CreateThread(NULL,0,install_worker,NULL,0,NULL);
    if(!t){say("could not start the installer thread");return 1;}
    CloseHandle(t);
    return 0;
}
