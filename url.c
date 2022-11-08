#include <stdio.h>
#include "postgres.h"

#include "access/gist.h"
#include "access/skey.h"
#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/builtins.h"
#include "libpq/pqformat.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/nabstime.h"
#include "utils/guc.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h> 

// magic module 
PG_MODULE_MAGIC;


typedef struct varlena url;


Datum url_in(PG_FUNCTION_ARGS);
Datum url_out(PG_FUNCTION_ARGS);
Datum url_recv(PG_FUNCTION_ARGS);
Datum url_send(PG_FUNCTION_ARGS);
Datum url_cast_to_text(PG_FUNCTION_ARGS);
Datum url_cast_from_text(PG_FUNCTION_ARGS);

