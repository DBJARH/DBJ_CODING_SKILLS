#ifndef DBJ_THE_GAME_DBJ_CONFIGURATOR_H
#define DBJ_THE_GAME_DBJ_CONFIGURATOR_H

// Where things are. Today that means asset files; the struct exists so the
// next question of the same kind lands here too, rather than as another
// literal path scattered through the code.
#include <dbj_result.h>
#include <dbj_str.h>

#include "dbj_platform.h"

// 512 is the whole path, not the leaf: "<exe_dir>/assets/<leaf>". Anything
// longer is reported, never truncated in silence.
DEFINE_DBJSTR_TYPE(dbj_str_512, 512)
DBJ_MAKERESULT(dbj_str_512);

typedef struct dbj_configurator dbj_configurator;

struct dbj_configurator {
	// "castle.txt" -> OK("<exe_dir>/assets/castle.txt").
	// ERR when the executable's directory is unknown or the path does not
	// fit; the ERR arm carries the origin and a sentence saying which.
	//
	// `cfg` is a plain pointer, not the repo's usual `cfg[static 1]`: a
	// member of dbj_configurator cannot name an *array* of the struct
	// still being defined, because an array needs a complete element
	// type. Every free function below keeps the array form.
	dbj_str_512Result (*assets_path)(dbj_configurator const *cfg,
	                                 char const *leaf);

	dbj_platform plat;

	// Resolved once, by the factory. Empty when resolution failed, which
	// is the state assets_path reports rather than papering over.
	dbj_str_512 root;
	bool        rooted;
};

dbj_configurator dbj_configurator_make(void);

#endif  // DBJ_THE_GAME_DBJ_CONFIGURATOR_H
