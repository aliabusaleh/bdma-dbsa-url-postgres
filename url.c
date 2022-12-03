#include <stdio.h>
#include "postgres.h"

#include "access/skey.h"
#include "utils/elog.h"
#include "utils/palloc.h"
#include "utils/builtins.h"
#include "libpq/pqformat.h"
#include "utils/date.h"
#include "utils/datetime.h"
// #include "utils/nabstime.h"
#include "utils/guc.h"
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include <uriparser/Uri.h>

// magic module
PG_MODULE_MAGIC;

typedef struct varlena url;
#define PG_RETURN_URL_P(x) PG_RETURN_POINTER(x)

Datum url_in(PG_FUNCTION_ARGS);
Datum url_out(PG_FUNCTION_ARGS);
// Datum url_recv(PG_FUNCTION_ARGS);
// Datum url_send(PG_FUNCTION_ARGS);
Datum url_cast_from_text(PG_FUNCTION_ARGS);

static char *toArray(int number)
{
	int n = log10(number) + 1;
	int i;
	char *numberArray = calloc(n, sizeof(char));
	for (i = n - 1; i >= 0; --i, number /= 10)
	{
		numberArray[i] = (number % 10) + '0';
	}
	return numberArray;
}

static bool
_is_host_set(UriUriA *url)
{
	return (url != NULL) && ((url->hostText.first != NULL) || (url->hostData.ip6 != NULL) || (url->hostData.ip4 != NULL) || (url->hostData.ipFuture.first != NULL));
}

static char *create_file(UriUriA url)
{
	StringInfoData buf;
	UriPathSegmentA *p;
	initStringInfo(&buf);
	if (url.absolutePath || (_is_host_set(&url) && url.pathHead))
		appendStringInfoChar(&buf, '/');

	for (p = url.pathHead; p; p = p->next)
	{
		appendBinaryStringInfo(&buf, p->text.first, p->text.afterLast - p->text.first);
		if (p->next)
			appendStringInfoChar(&buf, '/');
	}
	if (url.query.first)
	{
		appendStringInfoChar(&buf, '?');
		appendBinaryStringInfo(&buf, url.query.first, url.query.afterLast - url.query.first);
	}
	return buf.data;
}

static text *
uri_text_range_to_text(UriTextRangeA text_url)
{
	if (!text_url.first || !text_url.afterLast)
		return NULL;

	return cstring_to_text_with_len(text_url.first, text_url.afterLast - text_url.first);
}


static void
parse_url(const char *s, UriUriA *urip)
{
	UriParserStateA state;
	char * default_url;
	short l = strlen(s);
	char default_scheme[7]= "http://"; 
	if(strstr(s, "://"))
	{
		default_url = malloc((l) * sizeof(char));
		strcpy(default_url,s);
	}
	else {
		default_url = malloc((l+5) * sizeof(char));
		strcpy(default_url,default_scheme);
		strncat(default_url, s, l);
	}
	state.uri = urip;
	uriParseUriA(&state, default_url);

	switch (state.errorCode)
	{
	case URI_SUCCESS:
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

static int
retrieve_url_port_num(UriUriA *uriobj)
{
	if (!uriobj->portText.first || !uriobj->portText.afterLast || uriobj->portText.afterLast == uriobj->portText.first)
		return -1;
	return strtol(pnstrdup(uriobj->portText.first, uriobj->portText.afterLast - uriobj->portText.first),
				  NULL, 10);
}

static int
str_case_insesetive_compare(const char *s1, const char *s2, size_t n)
{
	while (n-- > 0)
	{
		unsigned char ch1 = (unsigned char)*s1++;
		unsigned char ch2 = (unsigned char)*s2++;

		if (ch1 != ch2)
		{
			// convert all to captital letter
			if (ch1 >= 'A' && ch1 <= 'Z')
				ch1 += 'a' - 'A';

			if (ch2 >= 'A' && ch2 <= 'Z')
				ch2 += 'a' - 'A';

			if (ch1 != ch2)
				return (int)ch1 - (int)ch2;
		}
		if (ch1 == 0 || ch2 == 0)
			break;
	}
	return 0;
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
cmp_hosts(UriUriA *uap, UriUriA *ubp)
{
	// if first host is null
	if (!uap->hostText.first)
	{
		// if second host is null
		if (!ubp->hostText.first) {

			return 0;
		}
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

static int cmp_nodes(struct UriPathSegmentStructA *a, struct UriPathSegmentStructA *b)
{
	int res = 0;
	while (a != NULL && b != NULL) {
		res = cmp_text_range(a->text, b->text);
		if(res != 0)
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

static int cmp_path(UriUriA *uap, UriUriA *ubp)
{
	// if first host is null
	int result;
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
	result = 0;
	result = cmp_text_range(uap->pathHead->text, ubp->pathHead->text);
	if (result == 0){

	struct UriPathSegmentStructA *uap_v2 = uap->pathHead->next;
	struct UriPathSegmentStructA *ubp_v2 = ubp->pathHead->next;
		result = cmp_nodes(uap_v2, ubp_v2);
	}

	// return the results anyway
	return result;

}

// Btree compere function
/*
 * IP4 is less than IP6
 * not null is positive, null is negaitve
 * text is bigger than Ip, if you compare www.google.com AND  129.231.222.8 --> then  www.google.com > 129.231.222.8
 * do we need to check port/whole string ?
 */
static int
url_cmp(Datum a, Datum b, bool btree)
{
	// elog(INFO, "Entering url_cmp!");
	char *sa = TextDatumGetCString(a);
	char *sb = TextDatumGetCString(b);
	UriUriA ua;
	UriUriA ub;
	int res = 0;

	parse_url(sa, &ua);
	parse_url(sb, &ub);

	// just remove fragments and query for Btree
	if (btree == true){
		// elog(INFO,"enter btree");
		// elog(INFO, "I url cmp start %s", sa);
		// elog(INFO, "I url cmp start %s", sb);
		// ignore fragment
		ua.fragment.first = NULL;
		ua.fragment.afterLast = NULL;

		ub.fragment.first = NULL;
		ub.fragment.afterLast = NULL;

		int charsRequired = 256;
		char * uriStringa;
		char * uriStringb;
		uriStringa = malloc(charsRequired * sizeof(char));
		uriStringb = malloc(charsRequired * sizeof(char));
		uriToStringA(uriStringa, &ua, charsRequired, NULL);
		uriToStringA(uriStringb, &ub, charsRequired, NULL);
		sa = uriStringa;
		sb = uriStringb;

	}


	if (res == 0)
		res = cmp_text_range(ua.scheme, ub.scheme);
    // at this poirnt, both of them has same schema
    // start checking hosts
	if (res == 0)
		res = cmp_hosts(&ua, &ub);
    // at this state, both schema and host are the same
    // check the path/(file)
	// where path is url path, and file is path+query
	if (res == 0)
		res = strcasecmp_ascii(sa, sb);
		//  elog(INFO, "strcasecmp_ascii res %d", res);
	if (res == 0)
		res = strcmp(sa, sb);
	uriFreeUriMembersA(&ua);
	uriFreeUriMembersA(&ub);
		// elog(INFO, "---------------------------------------------------------------");


	return res;
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
	new_text = malloc(strlen(s) + strlen(arg2) +1 + 10);
	strcpy(new_text, s);
	strcat(new_text, arg2);
	parse_url(new_text, &uri);
	uriFreeUriMembersA(&uri);
	vardata = (url*)cstring_to_text(new_text);
	PG_RETURN_URL_P(vardata);
}
PG_FUNCTION_INFO_V1(url_out);
Datum url_out(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);

	PG_RETURN_CSTRING(TextDatumGetCString(arg));
}

PG_FUNCTION_INFO_V1(get_host);
Datum get_host(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	text *result;

	parse_url(s, &url);
	result = uri_text_range_to_text(url.hostText);
	uriFreeUriMembersA(&url);
	// Check if host is present or not
	// If not return null
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_port);
Datum get_port(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	int result;
	parse_url(s, &url);
	result = retrieve_url_port_num(&url);

	uriFreeUriMembersA(&url);

	// Check if host is present or not
	// If not return null
	if (result)
		PG_RETURN_INT32(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_default_port);
Datum get_default_port(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	char *st;
	parse_url(s, &url);
	// Check if port is present or not
	// If not return -1
	if (!url.scheme.first ){
		PG_RETURN_INT32(-1);
	}
	st = TextDatumGetCString(uri_text_range_to_text(url.scheme));
	uriFreeUriMembersA(&url);
	if (strcmp(st, "http") == 0)
		PG_RETURN_INT32(80);
	else if (strcmp(st, "https") == 0)
		PG_RETURN_INT32(443);
	else if (strcmp(st, "ftp") == 0)
		PG_RETURN_INT32(21);
	else
		PG_RETURN_INT32(8080);
}

PG_FUNCTION_INFO_V1(get_protocol);
Datum get_protocol(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	text *result;
	parse_url(s, &url);
	result = uri_text_range_to_text(url.scheme);
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_query);
Datum get_query(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	text *result;
	parse_url(s, &url);
	result = uri_text_range_to_text(url.query);
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_user_info);
Datum get_user_info(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	text *result;
	parse_url(s, &url);
	result = uri_text_range_to_text(url.userInfo);
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_ref);
Datum get_ref(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0)){
		elog(WARNING, "No Ref found");
		PG_RETURN_NULL();
	}
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	text *result;
	parse_url(s, &url);
	result = uri_text_range_to_text(url.fragment);
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_NULL();
}

PG_FUNCTION_INFO_V1(get_path);
Datum get_path(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0)){
		elog(WARNING, "No Path found");
		PG_RETURN_NULL();
	}
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	StringInfoData buf;
	UriPathSegmentA *p;

	initStringInfo(&buf);

	parse_url(s, &url);

	if (url.absolutePath || (_is_host_set(&url) && url.pathHead))
		appendStringInfoChar(&buf, '/');

	for (p = url.pathHead; p; p = p->next)
	{
		appendBinaryStringInfo(&buf, p->text.first, p->text.afterLast - p->text.first);
		if (p->next)
			appendStringInfoChar(&buf, '/');
	}

	uriFreeUriMembersA(&url);
	PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}

PG_FUNCTION_INFO_V1(get_file);
Datum get_file(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0)){
		elog(WARNING, "No File found");
		PG_RETURN_NULL();
	}
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	StringInfoData buf;
	UriPathSegmentA *p;
	// UriTextRangeA text_url
	initStringInfo(&buf);

	parse_url(s, &url);

	if (url.absolutePath || (_is_host_set(&url) && url.pathHead))
		appendStringInfoChar(&buf, '/');

	for (p = url.pathHead; p; p = p->next)
	{
		appendBinaryStringInfo(&buf, p->text.first, p->text.afterLast - p->text.first);
		if (p->next)
			appendStringInfoChar(&buf, '/');
	}
	if (url.query.first)
	{
		appendStringInfoChar(&buf, '?');
		appendBinaryStringInfo(&buf, url.query.first, url.query.afterLast - url.query.first);
	}

	PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}

PG_FUNCTION_INFO_V1(get_authority);
Datum get_authority(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0)){
		elog(WARNING, "No Authority found");
		PG_RETURN_NULL();
	}
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	StringInfoData buf;
	// UriTextRangeA text_url
	initStringInfo(&buf);

	parse_url(s, &url);

	if (url.userInfo.first)
	{
		appendBinaryStringInfo(&buf, url.userInfo.first, url.userInfo.afterLast - url.userInfo.first);
		appendStringInfoChar(&buf, '@');
	}
	if (url.hostText.first)
	{
		appendBinaryStringInfo(&buf, url.hostText.first, url.hostText.afterLast - url.hostText.first);
	}
	if (url.portText.first)
	{
		appendStringInfoChar(&buf, ':');
		appendBinaryStringInfo(&buf, url.portText.first, url.portText.afterLast - url.portText.first);
	}

	PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}

PG_FUNCTION_INFO_V1(get_string);
Datum get_string(PG_FUNCTION_ARGS)
{
	Datum arg = PG_GETARG_DATUM(0);
	char *s = TextDatumGetCString(arg);
	UriUriA url;
	char *uriString;
	int charsRequired;
	parse_url(s, &url);
	if (uriToStringCharsRequiredA(&url, &charsRequired) == URI_SUCCESS)
	{
		PG_RETURN_NULL();
	}
	charsRequired++;
	uriString = malloc(charsRequired * sizeof(char));
	if (uriToStringA(uriString, &url, charsRequired, NULL) != URI_SUCCESS)
	{
		PG_RETURN_NULL();
	}
	PG_RETURN_TEXT_P(cstring_to_text(uriString));
}

PG_FUNCTION_INFO_V1(same_host);
Datum same_host(PG_FUNCTION_ARGS)
{
	// elog(INFO, "im here!");
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	char *s1 = TextDatumGetCString(arg1);
	char *s2 = TextDatumGetCString(arg2);
	// elog(INFO, "s1: %s, s2: %s", s1, s2);


	UriUriA ua;
	UriUriA ub;
	int res = 0;
	parse_url(s1, &ua);
	parse_url(s2, &ub);
	res = cmp_hosts(&ua, &ub);
	// elog(INFO, "res is %d", res);
	if (res == 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}

PG_FUNCTION_INFO_V1(same_url);
Datum same_url(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0) | PG_ARGISNULL(2)){
		elog(WARNING, "Two argument are required SameURL(text,text)");
		PG_RETURN_NULL();
	}
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	int res;
	char *sa = TextDatumGetCString(arg1);
	char *sb = TextDatumGetCString(arg2);
	res = strcasecmp_ascii(sa, sb);
	if (res == 0)
		res = strcmp(sa, sb);
	if(res == 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}

PG_FUNCTION_INFO_V1(same_file);
Datum same_file(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0) | PG_ARGISNULL(2)){
		elog(WARNING, "Two argument are required SameFile(text,text)");
		PG_RETURN_NULL();
	}
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	char *s1 = TextDatumGetCString(arg1);
	char *s2 = TextDatumGetCString(arg2);
	int res;
	UriUriA url1;
	UriUriA url2;
	parse_url(s1, &url1);
	parse_url(s2, &url2);
	char *sa = create_file(url1);
	char *sb = create_file(url2);
	res = strcasecmp_ascii(sa, sb);
	if (res == 0)
		res = strcmp(sa, sb);
	if(res == 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}


PG_FUNCTION_INFO_V1(url_abs_rt);
Datum url_abs_rt(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	int res;
	res  = url_cmp(arg1, arg2, false);
	
	if(res > 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}


PG_FUNCTION_INFO_V1(url_abs_lt);
Datum url_abs_lt(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	int res;
	res  = url_cmp(arg1, arg2, false);
	if(res < 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}


PG_FUNCTION_INFO_V1(url_rt);
Datum url_rt(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	int res;
	res  = url_cmp(arg1, arg2, false);
	if(res >= 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}

PG_FUNCTION_INFO_V1(url_lt);
Datum url_lt(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	int res;
	res  = url_cmp(arg1, arg2, false);
	if(res <= 0)
		PG_RETURN_BOOL(1);
	else
		PG_RETURN_BOOL(0);
}



PG_FUNCTION_INFO_V1(url_cmp_internal);
Datum url_cmp_internal(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	PG_RETURN_INT32(url_cmp(arg1, arg2, true));
}

PG_FUNCTION_INFO_V1(url_cmp_internal_same_host);
Datum url_cmp_internal_same_host(PG_FUNCTION_ARGS)
{
	Datum arg1 = PG_GETARG_DATUM(0);
	Datum arg2 = PG_GETARG_DATUM(1);
	char *s1 = TextDatumGetCString(arg1);
	char *s2 = TextDatumGetCString(arg2);

	UriUriA ua;
	UriUriA ub;
	int res = 0;
	parse_url(s1, &ua);
	parse_url(s2, &ub);
	res = cmp_hosts(&ua, &ub);
	PG_RETURN_INT32(res);
}



PG_FUNCTION_INFO_V1(url_cast_from_text);
Datum url_cast_from_text(PG_FUNCTION_ARGS){
	// elog(INFO, "inside cast!");
	char* s = PG_GETARG_CSTRING(0);
	// elog(INFO, "input is %s", s);
	url* vardata;
	UriUriA uri;
	parse_url(s, &uri);
	uriFreeUriMembersA(&uri);
	// elog(INFO, "output is %s", s);
	vardata = (url*)cstring_to_text(s);

	PG_RETURN_URL_P(vardata);
}

