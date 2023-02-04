//! wasm3-el.c
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <emacs-module.h>
#include <wasm3.h>

/* Declare mandatory GPL symbol.  */
int plugin_is_GPL_compatible;

bool
read_binary(char *filename, uint8_t **buffer, size_t *size) {
    FILE* in_file = fopen(filename, "rb");
    if (!in_file) {
        perror("fopen");
        return false;
    }

    struct stat sb;
    if (stat(filename, &sb) == -1) {
        perror("stat");
        return false;
    }
    *size = sb.st_size;
    *buffer = malloc(sb.st_size);
    fread(*buffer, sb.st_size, 1, in_file);
    fclose(in_file);

    return true;
}

/* https://phst.eu/emacs-modules.html#copy_string_contents */
static bool
copy_string_contents (emacs_env *env, emacs_value value,
                      char **buffer, size_t *size)
{
  ptrdiff_t buffer_size;
  if (!env->copy_string_contents (env, value, NULL, &buffer_size))
    return false;
  assert (env->non_local_exit_check (env) == emacs_funcall_exit_return);
  assert (buffer_size > 0);
  *buffer = malloc ((size_t) buffer_size);
  if (*buffer == NULL)
    {
      env->non_local_exit_signal (env, env->intern (env, "memory-full"),
                                  env->intern (env, "nil"));
      return false;
    }
  ptrdiff_t old_buffer_size = buffer_size;
  if (!env->copy_string_contents (env, value, *buffer, &buffer_size))
    {
      free (*buffer);
      *buffer = NULL;
      return false;
    }
  assert (env->non_local_exit_check (env) == emacs_funcall_exit_return);
  assert (buffer_size == old_buffer_size);
  *size = (size_t) (buffer_size - 1);
  return true;
}

/**
 * A simple wasm3 function.
 */
static emacs_value Fwasm3_test(emacs_env *env, int nargs, emacs_value args[],
                               void *data) {

  (void)data;
  assert(M3_VERSION_MAJOR == 0 && M3_VERSION_MINOR == 5 &&
         M3_VERSION_REV == 0 && "Tested only on version wasm3 0.5.0");

  char *module_name;
  size_t module_size;
  copy_string_contents(env, args[0], &module_name, &module_size);
  char *function_name;
  size_t function_name_size;
  copy_string_contents(env, args[1], &function_name, &function_name_size);
  uint8_t *wasm_bytes;
  size_t wasm_size;
  read_binary(module_name, &wasm_bytes, &wasm_size);

  IM3Environment e = m3_NewEnvironment();

  IM3Runtime r = m3_NewRuntime(e, 1024, NULL);

  IM3Module m;
  /* for now, we just hardcode the fib.c.wasm module at test level. */
  M3Result result = m3_ParseModule(e, &m, wasm_bytes, wasm_size);
  if (result) {
    /* printf("Error initializing module: %s\n", result); */
    return 1;
  }
  result = m3_LoadModule(r, m);
  if (result) {
    /* printf("Error loading module: %s\n", result); */
    return 1;
  }
  /* printf("%s\n", m3_GetModuleName(m)); */

  IM3Function f = NULL;
  result = m3_FindFunction(&f, r, function_name);
  if (result) {
    /* printf("Error finding function: %s\n", result); */
    return 1;
  }

  static uint32_t argbuff[10];
  static const void *argptrs[10];
  memset(argbuff, 0, sizeof(argbuff));
  memset(argptrs, 0, sizeof(argptrs));

  for (int i = 0; i < 10; i++) {
    argptrs[i] = &argbuff[i];
  }

  argbuff[0] = 5;

  result = m3_Call(f, 1, argptrs);
  if (result) {
    /* printf("Error calling function: %s\n", result); */
    return 1;
  }
  static uint64_t valbuff[10];
  static const void *valptrs[10];
  for (int i = 0; i < 1; i++) {
    valptrs[i] = &valbuff[i];
  }
  result = m3_GetResults(f, 1, valptrs);
  if (result) {
    /* printf("Error getting results: %s\n", result); */
    return 1;
  }
  /* printf("%lld\n", valbuff[0]); */

  return env->make_integer(env, valbuff[0]);
}

/* Bind NAME to FUN.  */
static void bind_function(emacs_env *env, const char *name, emacs_value Sfun) {
  /* Set the function cell of the symbol named NAME to SFUN using
     the 'fset' function.  */

  /* Convert the strings to symbols by interning them */
  emacs_value Qfset = env->intern(env, "fset");
  emacs_value Qsym = env->intern(env, name);

  /* Prepare the arguments array */
  emacs_value args[] = {Qsym, Sfun};

  /* Make the call (2 == nb of arguments) */
  env->funcall(env, Qfset, 2, args);
}

/* Provide FEATURE to Emacs.  */
static void provide(emacs_env *env, const char *feature) {
  /* call 'provide' with FEATURE converted to a symbol */

  emacs_value Qfeat = env->intern(env, feature);
  emacs_value Qprovide = env->intern(env, "provide");
  emacs_value args[] = {Qfeat};

  env->funcall(env, Qprovide, 1, args);
}

int emacs_module_init(struct emacs_runtime *ert) {
  emacs_env *env = ert->get_environment(ert);

  /* create a lambda (returns an emacs_value) */
  emacs_value fun = env->make_function(
      env, 2,      /* min. number of arguments */
      2,           /* max. number of arguments */
      Fwasm3_test, /* actual function pointer */
      "doc",       /* docstring */
      NULL         /* user pointer of your choice (data param in Fmymod_test) */
  );

  bind_function(env, "wasm3-test", fun);
  provide(env, "wasm3");

  /* loaded successfully */
  return 0;
}
