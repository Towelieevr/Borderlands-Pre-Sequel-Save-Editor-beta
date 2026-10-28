#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "plugin_common.h"
#include "tps.h"

#define GNAMES_OFF   0x2289960
#define GOBJECTS_OFF 0x2309E40
#define PE_OFF       0x1B8
#define O_OUTER 0x40
#define O_NAME  0x48
#define O_CLASS 0x50
#define P_OFFSET 0x7C
#define POOL_VALUE 0xA0
#define CURRENCY_STRIDE 20
#define CURRENCY_VALUE  0x0C
enum { CUR_CREDITS = 0, CUR_MOONSTONE = 1 };

typedef struct { uintptr_t data; int32_t count, max; } TArray;

static uint64_t g_base;
static uintptr_t g_pc, g_pri, g_xp_pool;
static int32_t g_off_skill = -1, g_off_currency = -1, g_off_tokens = -1, g_off_pool_owner = -1;
#define POOL_OWNER g_off_pool_owner

void tps_set_base(uint64_t base) { g_base = base; }

static TArray* gnames(void)   { return (TArray*)(g_base + GNAMES_OFF); }
static TArray* gobjects(void) { return (TArray*)(g_base + GOBJECTS_OFF); }

static const char* name_str(int32_t idx) {
    TArray* n = gnames();
    if (idx < 0 || idx >= n->count) return "";
    uintptr_t e = ((uintptr_t*)n->data)[idx];
    return e ? (const char*)(e + 0x10) : "";
}
static int32_t name_index(const char* s) {
    TArray* n = gnames();
    for (int32_t i = 0; i < n->count; i++) {
        uintptr_t e = ((uintptr_t*)n->data)[i];
        if (e && strcmp((const char*)(e + 0x10), s) == 0) return i;
    }
    return -1;
}
static inline int32_t obj_name(uintptr_t o)  { return *(int32_t*)(o + O_NAME); }
static inline uintptr_t obj_class(uintptr_t o) { return *(uintptr_t*)(o + O_CLASS); }
static inline uintptr_t obj_outer(uintptr_t o) { return *(uintptr_t*)(o + O_OUTER); }

static uintptr_t find_instance(int32_t cls_name, int32_t level_name) {
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (!o) continue;
        uintptr_t c = obj_class(o), out = obj_outer(o);
        if (c && obj_name(c) == cls_name && out && obj_name(out) == level_name) return o;
    }
    return 0;
}
static int32_t prop_offset(int32_t cls_name, int32_t prop_name) {
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (!o || obj_name(o) != prop_name) continue;
        uintptr_t out = obj_outer(o);
        if (out && obj_name(out) == cls_name) return *(int32_t*)(o + P_OFFSET);
    }
    return -1;
}

int tps_init(void) {
    if (!g_base || gobjects()->count <= 0 || gnames()->count <= 0) return -1;
    int32_t n_pc = name_index("WillowPlayerController"), n_pri = name_index("WillowPlayerReplicationInfo");
    int32_t n_lvl = name_index("PersistentLevel"), n_xp = name_index("ExperienceResourcePool");
    if (n_pc < 0 || n_pri < 0 || n_lvl < 0) return -1;
    g_pc = find_instance(n_pc, n_lvl);
    g_pri = find_instance(n_pri, n_lvl);
    if (!g_pc || !g_pri) return -1;
    if (g_off_skill < 0) {
        g_off_skill    = prop_offset(n_pri, name_index("GeneralSkillPoints"));
        g_off_currency = prop_offset(n_pri, name_index("Currency"));
        g_off_tokens   = prop_offset(n_pc,  name_index("BadassTokensAvailable"));
        g_off_pool_owner = prop_offset(name_index("ResourcePool"), name_index("AssociatedProvider"));
    }
    if (g_off_pool_owner < 0) return -1;
    g_xp_pool = 0;
    TArray* g = gobjects();
    for (int32_t i = 0; n_xp >= 0 && i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (o && obj_class(o) && obj_name(obj_class(o)) == n_xp && *(uintptr_t*)(o + POOL_OWNER) == g_pc) { g_xp_pool = o; break; }
    }
    return 0;
}

uintptr_t tps_player_controller(void) { return g_pc; }

uintptr_t tps_find_object(const char* cls, const char* name) {
    int32_t nc = name_index(cls), nn = name_index(name);
    if (nc < 0 || nn < 0) return 0;
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (o && obj_name(o) == nn && obj_class(o) && obj_name(obj_class(o)) == nc) return o;
    }
    return 0;
}
int tps_fonts(uintptr_t* out, int max) {
    int32_t nc = name_index("Font");
    if (nc < 0) return 0;
    TArray* g = gobjects();
    int n = 0;
    for (int32_t i = 0; i < g->count && n < max; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (o && obj_class(o) && obj_name(obj_class(o)) == nc) out[n++] = o;
    }
    return n;
}
const char* tps_object_name(uintptr_t o) { return o ? name_str(obj_name(o)) : ""; }
uintptr_t tps_find_instance(const char* cls) {
    int32_t nc = name_index(cls);
    if (nc < 0) return 0;
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (!o || !obj_class(o) || obj_name(obj_class(o)) != nc) continue;
        if (strncmp(name_str(obj_name(o)), "Default__", 9) == 0) continue;
        return o;
    }
    return 0;
}
int32_t tps_param_offset(uintptr_t fn, const char* name) {
    int32_t nn = name_index(name);
    if (!fn || nn < 0) return -1;
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (o && obj_outer(o) == fn && obj_name(o) == nn) return *(int32_t*)(o + P_OFFSET);
    }
    return -1;
}
uintptr_t tps_find_function(const char* owner, const char* name) {
    int32_t no = name_index(owner), nn = name_index(name), nf = name_index("Function");
    if (no < 0 || nn < 0 || nf < 0) return 0;
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (!o || obj_name(o) != nn) continue;
        uintptr_t c = obj_class(o), out = obj_outer(o);
        if (c && obj_name(c) == nf && out && obj_name(out) == no) return o;
    }
    return 0;
}

typedef void (*PE_t)(void* self, void* fn, void* parms);
void tps_process_event(uintptr_t obj, uintptr_t fn, void* parms) {
    PE_t f = (PE_t)(*(void***)obj)[PE_OFF / 8];
    f((void*)obj, (void*)fn, parms);
}

int tps_player_level(void) {
    static int32_t off = -2;
    if (off == -2) off = prop_offset(name_index("WillowPlayerReplicationInfo"), name_index("ExpLevel"));
    return (g_pri && off >= 0) ? *(int32_t*)(g_pri + off) : 1;
}

typedef struct { uintptr_t* data; int32_t count, max; } ObjArray;
static uintptr_t s_fn_spawn, s_fn_add, s_cdo_itempool;
static int32_t s_p_def, s_p_stage, s_p_awesome, s_p_ctx, s_p_out, s_p_add_inv, s_off_pawn = -1, s_off_invmgr = -1;

static int spawner_init(void) {
    if (s_fn_spawn) return 0;
    s_fn_spawn = tps_find_function("ItemPool", "SpawnBalancedInventoryFromPool");
    s_fn_add = tps_find_function("WillowInventoryManager", "AddInventoryToBackpack");
    s_cdo_itempool = tps_find_object("ItemPool", "Default__ItemPool");
    if (!s_fn_spawn || !s_fn_add || !s_cdo_itempool) { s_fn_spawn = 0; return -1; }
    s_p_def = tps_param_offset(s_fn_spawn, "Definition");
    s_p_stage = tps_param_offset(s_fn_spawn, "GameStage");
    s_p_awesome = tps_param_offset(s_fn_spawn, "AwesomeLevel");
    s_p_ctx = tps_param_offset(s_fn_spawn, "ContextSource");
    s_p_out = tps_param_offset(s_fn_spawn, "SpawnedInventory");
    s_p_add_inv = tps_param_offset(s_fn_add, "Inv");
    s_off_pawn = prop_offset(name_index("Controller"), name_index("Pawn"));
    s_off_invmgr = prop_offset(name_index("Pawn"), name_index("InvManager"));
    if (s_p_def < 0 || s_p_stage < 0 || s_p_ctx < 0 || s_p_out < 0 || s_p_add_inv < 0 || s_off_pawn < 0 || s_off_invmgr < 0) { s_fn_spawn = 0; return -2; }
    return 0;
}

uintptr_t tps_find_pool(const char* name) { return tps_find_object("ItemPoolDefinition", name); }

int tps_spawn_from_pool(uintptr_t pool, int level, int quantity) {
    if (!pool || !g_pc || spawner_init() != 0) return -1;
    uintptr_t pawn = *(uintptr_t*)(g_pc + s_off_pawn);
    if (!pawn) return -2;
    uintptr_t invmgr = *(uintptr_t*)(pawn + s_off_invmgr);
    if (!invmgr) return -3;
    int added = 0;
    for (int q = 0; q < quantity; q++) {
        uint8_t parms[0x60];
        memset(parms, 0, sizeof(parms));
        *(uintptr_t*)(parms + s_p_def) = pool;
        *(int32_t*)(parms + s_p_stage) = level;
        if (s_p_awesome >= 0) *(int32_t*)(parms + s_p_awesome) = 0;
        *(uintptr_t*)(parms + s_p_ctx) = pawn;
        tps_process_event(s_cdo_itempool, s_fn_spawn, parms);
        ObjArray* out = (ObjArray*)(parms + s_p_out);
        for (int32_t i = 0; out->data && i < out->count && i < 32; i++) {
            uintptr_t inv = out->data[i];
            if (!inv) continue;
            uint8_t ap[0x10];
            memset(ap, 0, sizeof(ap));
            *(uintptr_t*)(ap + s_p_add_inv) = inv;
            tps_process_event(invmgr, s_fn_add, ap);
            added++;
        }
    }
    return added;
}

static uintptr_t c_fn_create, c_cdo_inventory;
static int32_t c_p_serial, c_p_source, c_p_ret, c_off_slotmax = -1, c_off_count = -1;

static int b64val(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+' || c == '-') return 62;
    if (c == '/' || c == '_') return 63;
    return -1;
}
int tps_decode_code(const char* code, uint8_t* out, int max) {
    const char* p = strchr(code, '(');
    p = p ? p + 1 : code;
    uint32_t acc = 0; int bits = 0, n = 0;
    for (; *p && *p != ')'; p++) {
        if (*p == '=' ) break;
        int v = b64val(*p);
        if (v < 0) { if (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') continue; return -1; }
        acc = (acc << 6) | (uint32_t)v; bits += 6;
        if (bits >= 8) { bits -= 8; if (n >= max) return -1; out[n++] = (uint8_t)(acc >> bits); }
    }
    return n;
}

static int codes_init(void) {
    if (c_fn_create) return 0;
    c_fn_create = tps_find_function("WillowInventory", "CreateInventoryFromSerialNumber");
    c_cdo_inventory = tps_find_object("WillowInventory", "Default__WillowInventory");
    if (!c_fn_create || !c_cdo_inventory) { c_fn_create = 0; return -1; }
    c_p_serial = tps_param_offset(c_fn_create, "SerialNumber");
    c_p_source = tps_param_offset(c_fn_create, "InAdditionalQueryInterfaceSource");
    c_p_ret = tps_param_offset(c_fn_create, "ReturnValue");
    c_off_slotmax = prop_offset(name_index("WillowInventoryManager"), name_index("InventorySlotMax_Misc"));
    c_off_count = prop_offset(name_index("WillowInventoryManager"), name_index("BackpackInventoryCount"));
    if (c_p_serial < 0 || c_p_ret < 0) { c_fn_create = 0; return -2; }
    return 0;
}

static uintptr_t inventory_manager(void) {
    if (spawner_init() != 0 || !g_pc) return 0;
    uintptr_t pawn = *(uintptr_t*)(g_pc + s_off_pawn);
    return pawn ? *(uintptr_t*)(pawn + s_off_invmgr) : 0;
}

int tps_in_gameplay(void) { return tps_init() == 0 && inventory_manager() != 0; }

int tps_backpack_free(void) {
    uintptr_t im = inventory_manager();
    if (!im || codes_init() != 0 || c_off_slotmax < 0 || c_off_count < 0) return -1;
    return *(int32_t*)(im + c_off_slotmax) - *(int32_t*)(im + c_off_count);
}

#define DEF_GRADE 0x18
#define DEF_STAGE 0x78
static uintptr_t l_fn_init_w, l_fn_init_i;
static int32_t l_off_def_w = -1, l_off_def_i = -1, l_p_w[3], l_p_i[3], l_off_exp_w = -1, l_off_exp_i = -1;
static int32_t exp_level_offset(const char* cls) {
    int32_t n = name_index("ExpLevel"), o = prop_offset(name_index(cls), n);
    if (o < 0) o = prop_offset(name_index("WillowInventory"), n);
    if (o < 0) o = prop_offset(name_index("Inventory"), n);
    return o;
}
static int relevel_init(void) {
    if (l_fn_init_w) return 0;
    l_fn_init_w = tps_find_function("WillowWeapon", "InitializeFromDefinitionData");
    l_fn_init_i = tps_find_function("WillowItem", "InitializeFromDefinitionData");
    l_off_def_w = prop_offset(name_index("WillowWeapon"), name_index("DefinitionData"));
    l_off_def_i = prop_offset(name_index("WillowItem"), name_index("DefinitionData"));
    if (!l_fn_init_w || !l_fn_init_i || l_off_def_w < 0 || l_off_def_i < 0) { l_fn_init_w = 0; return -1; }
    const char* pn[3] = {"NewDefinitionData", "InAdditionalQueryInterfaceSource", "bForceSelectNameParts"};
    for (int k = 0; k < 3; k++) { l_p_w[k] = tps_param_offset(l_fn_init_w, pn[k]); l_p_i[k] = tps_param_offset(l_fn_init_i, pn[k]); }
    if (l_p_w[0] < 0 || l_p_i[0] < 0) { l_fn_init_w = 0; return -2; }
    l_off_exp_w = exp_level_offset("WillowWeapon");
    l_off_exp_i = exp_level_offset("WillowItem");
    return 0;
}
static int relevel(uintptr_t inv, bool weapon, int level, uintptr_t source) {
    if (relevel_init() != 0) return -1;
    int32_t off = weapon ? l_off_def_w : l_off_def_i;
    int32_t* p = weapon ? l_p_w : l_p_i;
    int32_t exp = weapon ? l_off_exp_w : l_off_exp_i;
    uint8_t parms[0xA0];
    memset(parms, 0, sizeof(parms));
    memcpy(parms + p[0], (void*)(inv + off), 128);
    *(int32_t*)(parms + p[0] + DEF_GRADE) = level;
    *(int32_t*)(parms + p[0] + DEF_STAGE) = level;
    memcpy((void*)(inv + off), parms + p[0], 128);
    if (p[1] >= 0) *(uintptr_t*)(parms + p[1]) = source;
    tps_process_event(inv, weapon ? l_fn_init_w : l_fn_init_i, parms);
    if (exp >= 0 && *(int32_t*)(inv + exp) != level) *(int32_t*)(inv + exp) = level;
    return 0;
}

int tps_unlock_all_cosmetics(void) {
    if (!g_pc) return -1;
    uintptr_t mgr = tps_find_instance("WillowCustomizationManager");
    uintptr_t fn_lock = tps_find_function("WillowCustomizationManager", "SetCustomizationLocked");
    uintptr_t fn_save = tps_find_function("WillowPlayerController", "WriteCustomizationUnlocksToProfile");
    int32_t n_def = name_index("CustomizationDefinition");
    if (!mgr || !fn_lock || n_def < 0) return -2;
    int32_t pd = tps_param_offset(fn_lock, "Definition"), pc = tps_param_offset(fn_lock, "Controller"), pl = tps_param_offset(fn_lock, "bLocked");
    if (pd < 0 || pc < 0 || pl < 0) return -3;
    int count = 0;
    TArray* g = gobjects();
    for (int32_t i = 0; i < g->count; i++) {
        uintptr_t o = ((uintptr_t*)g->data)[i];
        if (!o || !obj_class(o) || obj_name(obj_class(o)) != n_def) continue;
        if (strncmp(name_str(obj_name(o)), "Default__", 9) == 0) continue;
        uint8_t parms[0x20];
        memset(parms, 0, sizeof(parms));
        *(uintptr_t*)(parms + pd) = o;
        *(uintptr_t*)(parms + pc) = g_pc;
        *(uint32_t*)(parms + pl) = 0;
        tps_process_event(mgr, fn_lock, parms);
        count++;
    }
    if (fn_save) { uint8_t none[8] = {0}; tps_process_event(g_pc, fn_save, none); }
    return count;
}

int tps_spawn_code(const char* code, int level) {
    uint8_t bytes[40];
    int len = tps_decode_code(code, bytes, sizeof(bytes));
    if (len < 6 || (bytes[0] & 0x7F) != 7) return -1;
    if (codes_init() != 0) return -2;
    uintptr_t im = inventory_manager();
    if (!im) return -2;
    int freeslots = tps_backpack_free();
    if (freeslots == 0) return -4;
    uint8_t parms[0x50];
    memset(parms, 0, sizeof(parms));
    uint8_t* sn = parms + c_p_serial;
    memset(sn, 0xFF, 40);
    memcpy(sn, bytes, len);
    sn[0x28] = 4;
    *(int32_t*)(sn + 0x2C) = 0;
    *(int32_t*)(sn + 0x30) = len;
    if (c_p_source >= 0) *(uintptr_t*)(parms + c_p_source) = *(uintptr_t*)(g_pc + s_off_pawn);
    tps_process_event(c_cdo_inventory, c_fn_create, parms);
    uintptr_t inv = *(uintptr_t*)(parms + c_p_ret);
    if (!inv) return -3;
    if (level > 0) relevel(inv, (bytes[0] & 0x80) != 0, level, *(uintptr_t*)(g_pc + s_off_pawn));
    uint8_t ap[0x10];
    memset(ap, 0, sizeof(ap));
    *(uintptr_t*)(ap + s_p_add_inv) = inv;
    tps_process_event(im, s_fn_add, ap);
    return 1;
}

static int get_i32(uintptr_t obj, int32_t off, int64_t* out) {
    if (!obj || off < 0) return -1; *out = *(int32_t*)(obj + off); return 0;
}
static int set_i32(uintptr_t obj, int32_t off, int64_t v) {
    if (!obj || off < 0) return -1; *(int32_t*)(obj + off) = (int32_t)v; return 0;
}
static int get_currency(int idx, int64_t* out) {
    if (!g_pri || g_off_currency < 0) return -1;
    *out = *(int32_t*)(g_pri + g_off_currency + idx * CURRENCY_STRIDE + CURRENCY_VALUE); return 0;
}
static int set_currency(int idx, int64_t v) {
    if (!g_pri || g_off_currency < 0) return -1;
    uintptr_t e = g_pri + g_off_currency + idx * CURRENCY_STRIDE;
    *(int32_t*)(e + CURRENCY_VALUE) = (int32_t)v;
    *(int32_t*)(e + CURRENCY_VALUE + 4) = (int32_t)v;
    return 0;
}

int tps_get_money(int64_t* o)          { return get_currency(CUR_CREDITS, o); }
int tps_set_money(int64_t v)           { return set_currency(CUR_CREDITS, v); }
int tps_get_moonstone(int64_t* o)      { return get_currency(CUR_MOONSTONE, o); }
int tps_set_moonstone(int64_t v)       { return set_currency(CUR_MOONSTONE, v); }
int tps_get_skill_points(int64_t* o)   { return get_i32(g_pri, g_off_skill, o); }
int tps_set_skill_points(int64_t v)    { return set_i32(g_pri, g_off_skill, v); }
int tps_get_badass_tokens(int64_t* o)  { return get_i32(g_pc, g_off_tokens, o); }
int tps_set_badass_tokens(int64_t v)   { return set_i32(g_pc, g_off_tokens, v); }
int tps_get_xp(int64_t* o) {
    if (!g_xp_pool) return -1; *o = (int64_t)*(float*)(g_xp_pool + POOL_VALUE); return 0;
}
static uintptr_t x_fn_apply, x_fn_maxlevel, x_fn_pawn_setlevel;
static int32_t x_p_maxret = -1, x_p_newlevel = -1;
static void xp_init(void) {
    if (x_fn_apply) return;
    x_fn_maxlevel = tps_find_function("WillowPlayerController", "GetMaxExpLevel");
    x_p_maxret = tps_param_offset(x_fn_maxlevel, "ReturnValue");
    x_fn_pawn_setlevel = tps_find_function("WillowPlayerPawn", "SetExpLevel");
    x_p_newlevel = tps_param_offset(x_fn_pawn_setlevel, "NewExpLevel");
    x_fn_apply = tps_find_function("ExperienceResourcePool", "ApplyExpPointsToExpLevel");
}
static void apply_xp(void) {
    xp_init();
    if (!x_fn_apply || !g_xp_pool) return;
    uint8_t p[0x10]; memset(p, 0, sizeof(p));
    tps_process_event(g_xp_pool, x_fn_apply, p);
}
int tps_set_xp(int64_t v) {
    if (!g_xp_pool) return -1; *(float*)(g_xp_pool + POOL_VALUE) = (float)v; apply_xp(); return 0;
}

static double ln_d(double x) {
    int k = 0; while (x > 2.0) { x /= 2.0; k++; } while (x < 1.0) { x *= 2.0; k--; }
    double y = (x - 1) / (x + 1), y2 = y * y, t = y, sum = 0;
    for (int n = 1; n < 60; n += 2) { sum += t / n; t *= y2; }
    return 2 * sum + k * 0.69314718055994530942;
}
static double exp_d(double x) {
    int k = 0; while (x > 0.5) { x /= 2; k++; }
    double sum = 1, t = 1;
    for (int n = 1; n < 30; n++) { t *= x / n; sum += t; }
    while (k--) sum *= sum;
    return sum;
}
int64_t tps_xp_for_level(int level) {
    if (level <= 1) return 0;
    double v = 60.0 * exp_d(2.8 * ln_d((double)level)) - 60.0;
    int64_t i = (int64_t)v;
    return (double)i < v ? i + 1 : i;
}
int tps_max_level(void) {
    xp_init();
    if (!g_pc || !x_fn_maxlevel || x_p_maxret < 0) return 70;
    uint8_t p[0x10]; memset(p, 0, sizeof(p));
    tps_process_event(g_pc, x_fn_maxlevel, p);
    int mx = *(int32_t*)(p + x_p_maxret);
    return mx >= 1 && mx <= 100 ? mx : 70;
}
int tps_get_level(int64_t* o) { if (!g_pri) return -1; *o = tps_player_level(); return 0; }
int tps_set_level(int64_t v) {
    if (!g_xp_pool || !g_pri || spawner_init() != 0) return -1;
    int mx = tps_max_level();
    int lv = v < 1 ? 1 : v > mx ? mx : (int)v;
    int cur = tps_player_level();
    int64_t xp = tps_xp_for_level(lv);
    *(float*)(g_xp_pool + POOL_VALUE) = (float)xp;
    if (lv > cur) {
        apply_xp();
        for (int k = 0; k < 3 && tps_player_level() < lv; k++) {
            xp += xp / 500 + 1;
            *(float*)(g_xp_pool + POOL_VALUE) = (float)xp;
            apply_xp();
        }
    } else if (lv < cur) {
        uintptr_t pawn = *(uintptr_t*)(g_pc + s_off_pawn);
        xp_init();
        if (pawn && x_fn_pawn_setlevel && x_p_newlevel >= 0) {
            uint8_t p[0x10]; memset(p, 0, sizeof(p));
            *(int32_t*)(p + x_p_newlevel) = lv;
            tps_process_event(pawn, x_fn_pawn_setlevel, p);
        }
    }
    return tps_player_level() == lv ? 0 : -2;
}

int tps_sync_backpack_level(void) {
    static int32_t off_bp = -2, n_weapon = -1;
    if (off_bp == -2) { off_bp = prop_offset(name_index("WillowInventoryManager"), name_index("Backpack")); n_weapon = name_index("WillowWeapon"); }
    uintptr_t im = inventory_manager();
    if (!im || off_bp < 0) return -1;
    int level = tps_player_level(), n = 0;
    uintptr_t pawn = *(uintptr_t*)(g_pc + s_off_pawn);
    ObjArray* bp = (ObjArray*)(im + off_bp);
    for (int32_t i = 0; bp->data && i < bp->count && i < 200; i++) {
        uintptr_t inv = bp->data[i];
        if (!inv || !obj_class(inv)) continue;
        if (relevel(inv, obj_name(obj_class(inv)) == n_weapon, level, pawn) == 0) n++;
    }
    return n;
}

#define N_AMMO 8
static const char* k_ammo_res[N_AMMO] = {"Ammo_Combat_Rifle", "Ammo_Repeater_Pistol", "Ammo_Patrol_SMG", "Ammo_Combat_Shotgun",
                                         "Ammo_Sniper_Rifle", "Ammo_Rocket_Launcher", "Ammo_Combat_Laser", "Ammo_Grenade_Protean"};
static uintptr_t u_fn_bank, u_fn_ready, u_fn_calc;
static const char* k_ammo_bm[N_AMMO] = {"Upgrades_Ammo_AssaultRifle", "Upgrades_Ammo_Pistol", "Upgrades_Ammo_SMG", "Upgrades_Ammo_Shotgun",
                                        "Upgrades_Ammo_SniperRifles", "Upgrades_Ammo_RocketLauncher", "Upgrades_Ammo_Laser", "Upgrades_Ammo_Grenades"};
static int32_t u_off_ready = -1, u_off_bank = -1, u_off_chest = -1, u_off_maxslots = -1, u_off_bmarr = -1,
               u_off_pooldef = -1, u_off_res = -1, u_off_total = -1, u_off_bmidx = -1,
               u_p_bank = -1, u_p_ready = -1, u_p_calc = -1;

static int sdu_init(void) {
    if (u_fn_bank) return 0;
    if (codes_init() != 0 || c_off_slotmax < 0) return -1;
    int32_t n_im = name_index("WillowInventoryManager"), n_st = name_index("WillowInventoryStorage");
    int32_t n_rp = name_index("ResourcePool"), n_rpd = name_index("ResourcePoolDefinition");
    u_off_ready = prop_offset(n_im, name_index("WeaponReadyMax"));
    u_off_bank = prop_offset(n_im, name_index("TheBank"));
    u_off_chest = prop_offset(n_st, name_index("ChestSlots"));
    u_off_maxslots = prop_offset(n_st, name_index("MaxSlots"));
    u_off_bmarr = prop_offset(name_index("WillowPlayerController"), name_index("BlackMarketUpgrades"));
    u_off_pooldef = prop_offset(n_rp, name_index("Definition"));
    u_off_res = prop_offset(n_rpd, name_index("Resource"));
    u_off_total = prop_offset(n_rpd, name_index("TotalUpgradeCount"));
    u_off_bmidx = prop_offset(name_index("BlackMarketUpgradeDefinition"), name_index("CachedUpgradeIndex"));
    u_fn_ready = tps_find_function("WillowInventoryManager", "SetWeaponReadyMax");
    u_fn_calc = tps_find_function("ResourcePool", "CalculateBaseValues");
    uintptr_t fb = tps_find_function("WillowInventoryManager", "ClientSetBankSlots");
    u_p_bank = tps_param_offset(fb, "NewSlotCount");
    u_p_ready = tps_param_offset(u_fn_ready, "NewSize");
    u_p_calc = tps_param_offset(u_fn_calc, "bOnlyCalculateAttributeInitializedState");
    if (!fb || u_p_bank < 0 || u_off_bank < 0 || u_off_chest < 0 || u_off_pooldef < 0 || u_off_res < 0) return -2;
    u_fn_bank = fb;
    return 0;
}

static uintptr_t player_bank(uintptr_t im) { return u_off_bank >= 0 ? *(uintptr_t*)(im + u_off_bank) : 0; }

static uintptr_t s_pool[N_AMMO], s_res[N_AMMO];
static int32_t s_bm_idx[N_AMMO] = {-1, -1, -1, -1, -1, -1, -1, -1}, s_n_ammo_cls = -1, s_n_pool_cls = -1;
static bool is_pool(uintptr_t o) {
    uintptr_t c = obj_class(o);
    return c && (obj_name(c) == s_n_ammo_cls || obj_name(c) == s_n_pool_cls);
}

void tps_sdu_refresh(void) {
    memset(s_pool, 0, sizeof(s_pool)); memset(s_res, 0, sizeof(s_res)); memset(s_bm_idx, 0xFF, sizeof(s_bm_idx));
    if (sdu_init() != 0 || !g_pc) return;
    int32_t n_res[N_AMMO];
    s_n_ammo_cls = name_index("AmmoResourcePool");
    s_n_pool_cls = name_index("ResourcePool");
    for (int i = 0; i < N_AMMO; i++) n_res[i] = name_index(k_ammo_res[i]);
    TArray* g = gobjects();
    for (int32_t k = 0; k < g->count; k++) {
        uintptr_t o = ((uintptr_t*)g->data)[k];
        if (!o || !is_pool(o) || *(uintptr_t*)(o + POOL_OWNER) != g_pc) continue;
        uintptr_t def = *(uintptr_t*)(o + u_off_pooldef);
        uintptr_t res = def ? *(uintptr_t*)(def + u_off_res) : 0;
        for (int i = 0; res && i < N_AMMO; i++) if (obj_name(res) == n_res[i]) { s_pool[i] = o; s_res[i] = res; }
    }
    for (int i = 0; i < N_AMMO; i++) {
        uintptr_t up = tps_find_object("BlackMarketUpgradeDefinition", k_ammo_bm[i]);
        s_bm_idx[i] = (up && u_off_bmidx >= 0) ? *(int32_t*)(up + u_off_bmidx) : -1;
    }
}
static uintptr_t ammo_pool(int i, uintptr_t* res_out) {
    uintptr_t o = s_pool[i];
    if (!o || !is_pool(o) || *(uintptr_t*)(o + POOL_OWNER) != g_pc) return 0;
    if (res_out) *res_out = s_res[i];
    return o;
}
static int32_t* ammo_level_slot(int i) {
    if (u_off_bmarr < 0 || !g_pc) return NULL;
    TArray* bm = (TArray*)(g_pc + u_off_bmarr);
    int32_t idx = s_bm_idx[i];
    return (idx >= 0 && idx < bm->count && bm->data) ? (int32_t*)bm->data + idx : NULL;
}

int tps_sdu_max(int which) {
    if (sdu_init() != 0) return -1;
    uintptr_t im = inventory_manager();
    if (!im) return -1;
    switch (which) {
    case SDU_BACKPACK: return 39;
    case SDU_WEAPONS:  return 4;
    case SDU_BANK: { uintptr_t b = player_bank(im); return (b && u_off_maxslots >= 0) ? *(int32_t*)(b + u_off_maxslots) : -1; }
    default: {
        int mx = -1;
        for (int k = 0; k <= N_AMMO && mx < 0; k++) {
            uintptr_t pool = ammo_pool(k == 0 ? which - SDU_AMMO_AR : k - 1, NULL);
            uintptr_t def = pool ? *(uintptr_t*)(pool + u_off_pooldef) : 0;
            if (def && u_off_total >= 0) mx = *(int32_t*)(def + u_off_total);
        }
        return mx;
    } }
}

int tps_sdu_get(int which, int64_t* out) {
    if (sdu_init() != 0) return -1;
    uintptr_t im = inventory_manager();
    if (!im) return -1;
    switch (which) {
    case SDU_BACKPACK: *out = *(int32_t*)(im + c_off_slotmax); return 0;
    case SDU_WEAPONS:  if (u_off_ready < 0) return -1; *out = *(int32_t*)(im + u_off_ready); return 0;
    case SDU_BANK: { uintptr_t b = player_bank(im); if (!b) return -1; *out = *(int32_t*)(b + u_off_chest); return 0; }
    default: {
        int32_t* lv = ammo_level_slot(which - SDU_AMMO_AR);
        if (!lv) return -1;
        *out = *lv; return 0;
    } }
}

int tps_sdu_set(int which, int64_t v) {
    if (sdu_init() != 0) return -1;
    uintptr_t im = inventory_manager();
    if (!im) return -1;
    int mx = tps_sdu_max(which);
    if (mx >= 0 && v > mx) v = mx;
    if (v < 0) v = 0;
    uint8_t p[0x10]; memset(p, 0, sizeof(p));
    switch (which) {
    case SDU_BACKPACK: *(int32_t*)(im + c_off_slotmax) = (int32_t)v; return 0;
    case SDU_BANK:
        *(int32_t*)(p + u_p_bank) = (int32_t)v;
        tps_process_event(im, u_fn_bank, p);
        return 0;
    case SDU_WEAPONS:
        if (!u_fn_ready || u_p_ready < 0) return -1;
        *(int32_t*)(p + u_p_ready) = (int32_t)v;
        tps_process_event(im, u_fn_ready, p);
        return 0;
    default: {
        int32_t* lv = ammo_level_slot(which - SDU_AMMO_AR);
        if (!lv) return -1;
        *lv = (int32_t)v;
        uintptr_t pool = ammo_pool(which - SDU_AMMO_AR, NULL);
        if (pool && u_fn_calc && u_p_calc >= 0) { *(uint32_t*)(p + u_p_calc) = 1; tps_process_event(pool, u_fn_calc, p); }
        return 0;
    } }
}

int tps_sdu_max_all(void) {
    int n = 0;
    for (int i = 0; i < N_SDU; i++) { int mx = tps_sdu_max(i); if (mx > 0 && tps_sdu_set(i, mx) == 0) n++; }
    return n;
}
