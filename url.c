#include <stdio.h>
#include "postgres.h"

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

#include <uriparser/Uri.h>


// magic module 
PG_MODULE_MAGIC;

typedef struct varlena url;
#define PG_RETURN_URL_P(x)	PG_RETURN_POINTER(x)




Datum url_in(PG_FUNCTION_ARGS);
Datum url_out(PG_FUNCTION_ARGS);
// Datum url_recv(PG_FUNCTION_ARGS);
// Datum url_send(PG_FUNCTION_ARGS);
Datum url_cast_to_text(PG_FUNCTION_ARGS);
Datum url_cast_from_text(PG_FUNCTION_ARGS);




static char* toArray(int number)
{
	int n = log10(number) + 1;
	int i;
	char* numberArray = calloc(n, sizeof(char));
	for (i = n - 1; i >= 0; --i, number /= 10)
	{
		numberArray[i] = (number % 10) + '0';
	}
	return numberArray;
}

static void
parse_url(const char* s, UriUriA* urip)
{
	UriParserStateA state;

	state.uri = urip;
	uriParseUriA(&state, s);

	switch (state.errorCode)
	{
	case URI_SUCCESS:
		elog(INFO, "I am here %s", s);
		return;
	case URI_ERROR_SYNTAX:
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				errmsg("invalid input syntax for type uri at or near \"%s\"",
					state.errorPos)));
		break;
	default:
		elog(ERROR, "liburiparser error code %d", state.errorCode);
	}
}



PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
	char* s = PG_GETARG_CSTRING(0);
	url* vardata;
	UriUriA uri;
	parse_url(s, &uri);
	uriFreeUriMembersA(&uri);
	vardata = (url*)cstring_to_text(s);
	PG_RETURN_URL_P(vardata);
}


PG_FUNCTION_INFO_V1(url_in_part_two);
Datum
url_in_part_two(PG_FUNCTION_ARGS)
{
	url* vardata;
	UriUriA uri;
	char* arg1 = PG_GETARG_CSTRING(0);
	char* arg2 = PG_GETARG_CSTRING(1);
	int32 arg3 = PG_GETARG_INT32(2);
	char* arg4 = PG_GETARG_CSTRING(3);
	char* port = toArray(arg3);
	char* new_text;
	new_text = malloc(strlen(arg1) + strlen(arg2) + strlen(port) + strlen(arg4) + 1 + 10);
	strcpy(new_text, arg1);
	strcat(new_text, "://");
	strcat(new_text, arg2);
	strcat(new_text, ":");
	strcat(new_text, port);
	strcat(new_text, "/");
	strcat(new_text, arg4);
	parse_url(new_text, &uri);
	uriFreeUriMembersA(&uri);
	vardata = (url*)cstring_to_text(new_text);
	PG_RETURN_URL_P(vardata);
}

PG_FUNCTION_INFO_V1(url_in_part_three);
Datum
url_in_part_three(PG_FUNCTION_ARGS)
{
	url* vardata;
	UriUriA uri;
	char* arg1 = PG_GETARG_CSTRING(0);
	char* arg2 = PG_GETARG_CSTRING(1);
	char* arg3 = PG_GETARG_CSTRING(2);
	char* new_text;
	new_text = malloc(strlen(arg1) + strlen(arg2) + strlen(arg3) + 1 + 10);
	strcpy(new_text, arg1);
	strcat(new_text, "://");
	strcat(new_text, arg2);
	strcat(new_text, "/");
	strcat(new_text, arg3);
	parse_url(new_text, &uri);
	uriFreeUriMembersA(&uri);
	vardata = (url*)cstring_to_text(new_text);
	PG_RETURN_URL_P(vardata);
}

PG_FUNCTION_INFO_V1(url_in_part_four);
Datum
url_in_part_four(PG_FUNCTION_ARGS)
{
	url* vardata;
	UriUriA uri;
	Datum arg1 = PG_GETARG_DATUM(0);
	char* arg2 = PG_GETARG_CSTRING(1);
	char* s = TextDatumGetCString(arg1);
	char* new_text;
	new_text = malloc(strlen(s) + strlen(arg2) + +1 + 10);
	strcpy(new_text, s);
	strcat(new_text, arg2);
	parse_url(new_text, &uri);
	uriFreeUriMembersA(&uri);
	vardata = (url*)cstring_to_text(new_text);
	PG_RETURN_URL_P(vardata);
}

PG_FUNCTION_INFO_V1(url_out);
Datum
url_out(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(arg));
}


// Btree compere function 
/*
* IP4 is less than IP6 
* not null is positive, null is negaitve
* text is bigger than Ip, if you compare www.google.com AND  129.231.222.8 --> then  www.google.com > 129.231.222.8
* do we need to check port/whole string ? 
*/
static int
url_cmp(Datum a, Datum b)
{
	const char *sa = TextDatumGetCString(a);
	const char *sb = TextDatumGetCString(b);
	UriUriA ua;
	UriUriA ub;
	int res = 0;

	parse_uri(sa, &ua);
	parse_uri(sb, &ub);

	if (res == 0)
		res = cmp_text_range(ua.scheme, ub.scheme);
    // at this poirnt, both of them has same schema 
    // start checking hosts 
	if (res == 0)
		res = cmp_hosts(&ua, &ub);
    // at this state, both schema and host are the same 
    // check the path/(file)
    if (res == 0)
        res = cmp_path(&ua, &ub);
    // check the whole URL (File and fragments)
	if (res == 0)
		res = strcasecmp_ascii(sa, sb);
	if (res == 0)
		res = strcmp(sa, sb);
	uriFreeUriMembersA(&ua);
	uriFreeUriMembersA(&ub);

	return res;
}


static int cmp_path(UriUriA *uap, UriUriA *ubp)
{
	// if first host is null
	if(!uap->pathHead->text.first){
		// if second host is null 
		if(!ubp->pathHead->text.first){
			// both of them are null
			return 0;
		}
		// first is null, but second is not null 
		else {
			return -1;
		}
	}
	// first one isn't null, but second one is null
	else if (!ubp->pathHead->text.first){
		return 1;
	}
	
	// at this stage both are not null, we need to compare 
	int result = 0;
	result = cmp_text_range(uap->pathHead->text, ubp->pathHead->text);
	if (result == 0){

	struct UriPathSegmentStructA *uap_v2 = uap->pathHead->next;
	struct UriPathSegmentStructA *ubp_v2 = ubp->pathHead->next;
		result = cmp_nodes(uap_v2, ubp_v2)
	}

	// return the results anyway 
	return result;

}

static int cmp_nodes(struct UriPathSegmentStructA *a, struct UriPathSegmentStructA *b)
{
	int res = 0 
	while (a != NULL && b != NULL) {
		res = cmp_text_range(a->text, b->text)
		if res != 0
		return res;
        /* If we reach here, then a and b are not NULL and
           their data is same, so move to next nodes in both
           lists */
		a = a->next;
		b = b->next;
	}
	// maybe of of them are is shorter than the other 
	// case a is longer ( and b reached null )
	if (a != NULL) return -1;
	// case b is longer ( and a reached null )
    if (b != NULL) return 1;
    // If linked lists are identical, then 'a' and 'b' must
    // be NULL at this point.
	// return 0 
	return 0;
}
static int
cmp_hosts(UriUriA *uap, UriUriA *ubp)
{
    // if first host is null 
	if (!uap->hostText.first)
	{
        // if second host is null 
		if (!ubp->hostText.first)
        // both of them are null 
			return 0;
		else
        // first one is null, but second is not null 
			return -1;
	}
    // at this stage, first one is not null, 
    // case first host has ip4 type 
	else if (uap->hostData.ip4)
	{   
        // if second host is null 
		if (!ubp->hostText.first)
            // at this stage, we have 1st url is not null, and it has ip4, and the second one is null 
			return 1;
        // checking ip4 for host 2 
		else if (ubp->hostData.ip4)
            // at this stage, we have 1st url is not null, and it has ip4, and the second also has ip4 
            // check the ip4 date if it's same or not 
			return memcmp(uap->hostData.ip4->data,
						  ubp->hostData.ip4->data,
						  sizeof(uap->hostData.ip4->data));
        // at this stage, host 2 is not null, but it's not using IP4, which means IP6, then IP4 is smaller than IP6 
		else
			return -1;
	}
    // case first host has IP6 
	else if (uap->hostData.ip6)
	{
        // if second host is null 
		if (!ubp->hostText.first)
			return 1;
        // if second host is using IP4 (less than IP6)
		else if (ubp->hostData.ip4)
			return 1;
        // at this stage, both are using IP6 
        // compare the values
		else if (ubp->hostData.ip6)
			return memcmp(uap->hostData.ip6->data,
						  ubp->hostData.ip6->data,
						  sizeof(uap->hostData.ip6->data));
        // 
		else
			return -1;
	}
    // if first host and second host, both has text at a host, compare the text, "case insesitave"
	else
		return cmp_text_range(uap->hostText, ubp->hostText);
}


/**
 * 
 * 
*/
static int
cmp_text_range(UriTextRangeA a, UriTextRangeA b)
{
    // if 1st char in 1st url is null
    // OR
    //  last char in 1st url is null 
	if (!a.first || !a.afterLast)
	{
        // if 1st char in 2nd url is null
        // OR
        //  last char in 2nd url is null 
		if (!b.first || !b.afterLast)
        // both of them are null ( we have no schema/host in both)
			return 0;
        // first one is null 
		else
        // 1st url has no schema/host, ( no need to check the second one "we already confimred it's not null") 
			return -1;
	}
    // first URL is not null at this point 
    // if second URL is null retrun 1 
	else if (!b.first || !b.afterLast)
		return 1;
    
    // at this stage, none of them is null, we need to check the schema itself 
	else
	{
        // checking the string char by char
		int x = str_case_insesetive_compare(a.first, b.first,
								  Min(a.afterLast - a.first, b.afterLast - b.first));
        // if one of them has bigger length, then return the len(a) - len(b)
		if (x == 0)
			return (a.afterLast - a.first) - (b.afterLast - b.first);
		return x;
	}
}



static int
str_case_insesetive_compare(const char *s1, const char *s2, size_t n)
{
	while (n-- > 0)
	{
		unsigned char ch1 = (unsigned char) *s1++;
		unsigned char ch2 = (unsigned char) *s2++;

		if (ch1 != ch2)
		{   
            // convert all to captital letter
			if (ch1 >= 'A' && ch1 <= 'Z')
				ch1 += 'a' - 'A';

			if (ch2 >= 'A' && ch2 <= 'Z')
				ch2 += 'a' - 'A';

			if (ch1 != ch2)
				return (int) ch1 - (int) ch2;
		}
		if (ch1 == 0  ||  ch2 == 0 )
			break;
	}
	return 0;
}





static int
strcasecmp_ascii(const char *s1, const char *s2)
{
	for (;;)
	{
		unsigned char ch1 = (unsigned char) *s1++;
		unsigned char ch2 = (unsigned char) *s2++;

		if (ch1 != ch2)
		{
			if (ch1 >= 'A' && ch1 <= 'Z')
				ch1 += 'a' - 'A';

			if (ch2 >= 'A' && ch2 <= 'Z')
				ch2 += 'a' - 'A';

			if (ch1 != ch2)
				return (int) ch1 - (int) ch2;
		}
		if (ch1 == 0)
			break;
	}
	return 0;
}
