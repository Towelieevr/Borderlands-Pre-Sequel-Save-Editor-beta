#pragma once
#include <stdint.h>

void tps_set_base(uint64_t base);
int  tps_init(void);
int  tps_in_gameplay(void);
uintptr_t tps_player_controller(void);
uintptr_t tps_find_object(const char* cls, const char* name);
int       tps_fonts(uintptr_t* out, int max);
const char* tps_object_name(uintptr_t o);
uintptr_t tps_find_function(const char* owner, const char* name);
uintptr_t tps_find_instance(const char* cls);
int32_t   tps_param_offset(uintptr_t fn, const char* name);
void      tps_process_event(uintptr_t obj, uintptr_t fn, void* parms);
int       tps_player_level(void);
uintptr_t tps_find_pool(const char* name);
int       tps_spawn_from_pool(uintptr_t pool, int level, int quantity);
int       tps_decode_code(const char* code, uint8_t* out, int max);
int       tps_spawn_code(const char* code, int level);
int       tps_unlock_all_cosmetics(void);
int       tps_backpack_free(void);

int tps_get_money(int64_t* out);          int tps_set_money(int64_t v);
int tps_get_moonstone(int64_t* out);      int tps_set_moonstone(int64_t v);
int tps_get_skill_points(int64_t* out);   int tps_set_skill_points(int64_t v);
int tps_get_badass_tokens(int64_t* out);  int tps_set_badass_tokens(int64_t v);
int tps_get_xp(int64_t* out);             int tps_set_xp(int64_t v);
int tps_get_level(int64_t* out);          int tps_set_level(int64_t v);
int     tps_max_level(void);
int64_t tps_xp_for_level(int level);
int     tps_sync_backpack_level(void);

enum { SDU_BACKPACK, SDU_BANK, SDU_WEAPONS, SDU_AMMO_AR, SDU_AMMO_PISTOL, SDU_AMMO_SMG, SDU_AMMO_SHOTGUN,
       SDU_AMMO_SNIPER, SDU_AMMO_LAUNCHER, SDU_AMMO_LASER, SDU_AMMO_GRENADE, N_SDU };
int tps_sdu_get(int which, int64_t* out);
int tps_sdu_set(int which, int64_t v);
int tps_sdu_max(int which);
int tps_sdu_max_all(void);
void tps_sdu_refresh(void);
