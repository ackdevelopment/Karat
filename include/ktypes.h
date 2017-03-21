#pragma once

#include<stdint.h>
#include<stdlib.h>

typedef uint16_t addr_t;	/* address datatype */

#define CALLBACK(ret, ...) \
struct { ret(*cb)(void *, ##__VA_ARGS__); void *data; }
#define SET_CB(c, fp, d) c.cb = fp; c.data = d;
#define MAKE_CB(type, fp, d) (type){ fp, d }
#define RAW_CB(type, fp) MAKE_CB(type, fp, NULL)

typedef void(*destroy_fp)(void *);
typedef void(*init_fp)(void *);

#define CALL_CB(x, ...) x.cb(x.data, ##__VA_ARGS__)

#define U(size) \
typedef uint ## size ## _t u ## size
#define I(size) \
typedef int ## size ## _t i ## size

U(8);
U(16);
U(32);
U(64);

I(8);
I(16);
I(32);
I(64);

#undef U
#undef I
