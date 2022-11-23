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
#define PG_RETURN_URI_P(x)	PG_RETURN_POINTER(x)




Datum url_in(PG_FUNCTION_ARGS);
Datum url_out(PG_FUNCTION_ARGS);
// Datum url_recv(PG_FUNCTION_ARGS);
// Datum url_send(PG_FUNCTION_ARGS);
Datum url_cast_to_text(PG_FUNCTION_ARGS);
Datum url_cast_from_text(PG_FUNCTION_ARGS);


static void
parse_uri(const char *s, UriUriA *urip)
{
	UriParserStateA state;

	state.uri = urip;
	uriParseUriA(&state, s);

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



PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
	char *s = PG_GETARG_CSTRING(0);
	url *vardata;
	UriUriA uri;

	parse_uri(s, &uri);
	uriFreeUriMembersA(&uri);

	vardata = (url *) cstring_to_text(s);
	PG_RETURN_URI_P(vardata);
}



PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
	/*char  *protocol = PG_GETARG_CSTRING(0);
	 char *host = PG_GETARG_CSTRING(1);
	 */
// Have a look at the strcat function.

// In particular, you could try this:

// const char* name = "hello";
// const char* extension = ".txt";

	// char* name_with_extension;
	// name_with_extension = malloc(strlen(protocol)+1+4); /* make space for the new string (should check the return value ...) */
	// strcpy(name_with_extension, protocol); /* copy name into the new var */
	// strcat(name_with_extension, extension); /* add the extension */
	// ...
	// free(name_with_extension);
	text  *arg1 = PG_GETARG_CSTRING(0);
    text  *arg2 = PG_GETARG_CSTRING(1);
    int32 arg1_size = VARSIZE_ANY_EXHDR(arg1);
    int32 arg2_size = VARSIZE_ANY_EXHDR(arg2);
    int32 new_text_size = arg1_size + arg2_size + VARHDRSZ;
    text *new_text = (text *) palloc(new_text_size);

    SET_VARSIZE(new_text, new_text_size);
    memcpy(VARDATA(new_text), VARDATA_ANY(arg1), arg1_size);
    memcpy(VARDATA(new_text) + arg1_size, VARDATA_ANY(arg2), arg2_size);

	
	
	url *vardata;
	UriUriA uri;

	parse_uri(s, &uri);
	uriFreeUriMembersA(&uri);

	vardata = (url *) cstring_to_text(s);
	PG_RETURN_URI_P(vardata);
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
// static int
// url_cmp(Datum a, Datum b)
// {
// 	const char *sa = TextDatumGetCString(a);
// 	const char *sb = TextDatumGetCString(b);
// 	UriUriA ua;
// 	UriUriA ub;
// 	int res = 0;

// 	parse_uri(sa, &ua);
// 	parse_uri(sb, &ub);

// 	if (res == 0)
// 		res = cmp_text_range(ua.scheme, ub.scheme);
//     // at this poirnt, both of them has same schema 
//     // start checking hosts 
// 	if (res == 0)
// 		res = cmp_hosts(&ua, &ub);
//     // at this state, both schema and host are the same 
//     // check the path/(file)
//     if (res == 0)
//         res = cmp_path(&ua, &ub);
//     // check the whole URL (File and fragments)
// 	if (res == 0)
// 		res = strcasecmp_ascii(sa, sb);
// 	if (res == 0)
// 		res = strcmp(sa, sb);
// 	uriFreeUriMembersA(&ua);
// 	uriFreeUriMembersA(&ub);

// 	return res;
// }


// static int cmp_path(UriUriA *uap, UriUriA *ubp)
// {
// // implement this 

// }

// static int
// cmp_hosts(UriUriA *uap, UriUriA *ubp)
// {
//     // if first host is null 
// 	if (!uap->hostText.first)
// 	{
//         // if second host is null 
// 		if (!ubp->hostText.first)
//         // both of them are null 
// 			return 0;
// 		else
//         // first one is null, but second is not null 
// 			return -1;
// 	}
//     // at this stage, first one is not null, 
//     // case first host has ip4 type 
// 	else if (uap->hostData.ip4)
// 	{   
//         // if second host is null 
// 		if (!ubp->hostText.first)
//             // at this stage, we have 1st url is not null, and it has ip4, and the second one is null 
// 			return 1;
//         // checking ip4 for host 2 
// 		else if (ubp->hostData.ip4)
//             // at this stage, we have 1st url is not null, and it has ip4, and the second also has ip4 
//             // check the ip4 date if it's same or not 
// 			return memcmp(uap->hostData.ip4->data,
// 						  ubp->hostData.ip4->data,
// 						  sizeof(uap->hostData.ip4->data));
//         // at this stage, host 2 is not null, but it's not using IP4, which means IP6, then IP4 is smaller than IP6 
// 		else
// 			return -1;
// 	}
//     // case first host has IP6 
// 	else if (uap->hostData.ip6)
// 	{
//         // if second host is null 
// 		if (!ubp->hostText.first)
// 			return 1;
//         // if second host is using IP4 (less than IP6)
// 		else if (ubp->hostData.ip4)
// 			return 1;
//         // at this stage, both are using IP6 
//         // compare the values
// 		else if (ubp->hostData.ip6)
// 			return memcmp(uap->hostData.ip6->data,
// 						  ubp->hostData.ip6->data,
// 						  sizeof(uap->hostData.ip6->data));
//         // 
// 		else
// 			return -1;
// 	}
//     // if first host and second host, both has text at a host, compare the text, "case insesitave"
// 	else
// 		return cmp_text_range(uap->hostText, ubp->hostText);
// }


// /**
//  * 
//  * 
// */
// static int
// cmp_text_range(UriTextRangeA a, UriTextRangeA b)
// {
//     // if 1st char in 1st url is null
//     // OR
//     //  last char in 1st url is null 
// 	if (!a.first || !a.afterLast)
// 	{
//         // if 1st char in 2nd url is null
//         // OR
//         //  last char in 2nd url is null 
// 		if (!b.first || !b.afterLast)
//         // both of them are null ( we have no schema/host in both)
// 			return 0;
//         // first one is null 
// 		else
//         // 1st url has no schema/host, ( no need to check the second one "we already confimred it's not null") 
// 			return -1;
// 	}
//     // first URL is not null at this point 
//     // if second URL is null retrun 1 
// 	else if (!b.first || !b.afterLast)
// 		return 1;
    
//     // at this stage, none of them is null, we need to check the schema itself 
// 	else
// 	{
//         // checking the string char by char
// 		int x = str_case_insesetive_compare(a.first, b.first,
// 								  Min(a.afterLast - a.first, b.afterLast - b.first));
//         // if one of them has bigger length, then return the len(a) - len(b)
// 		if (x == 0)
// 			return (a.afterLast - a.first) - (b.afterLast - b.first);
// 		return x;
// 	}
// }



// static int
// str_case_insesetive_compare(const char *s1, const char *s2, size_t n)
// {
// 	while (n-- > 0)
// 	{
// 		unsigned char ch1 = (unsigned char) *s1++;
// 		unsigned char ch2 = (unsigned char) *s2++;

// 		if (ch1 != ch2)
// 		{   
//             // convert all to captital letter
// 			if (ch1 >= 'A' && ch1 <= 'Z')
// 				ch1 += 'a' - 'A';

// 			if (ch2 >= 'A' && ch2 <= 'Z')
// 				ch2 += 'a' - 'A';

// 			if (ch1 != ch2)
// 				return (int) ch1 - (int) ch2;
// 		}
// 		if (ch1 == 0  ||  ch2 == 0 )
// 			break;
// 	}
// 	return 0;
// }





// static int
// strcasecmp_ascii(const char *s1, const char *s2)
// {
// 	for (;;)
// 	{
// 		unsigned char ch1 = (unsigned char) *s1++;
// 		unsigned char ch2 = (unsigned char) *s2++;

// 		if (ch1 != ch2)
// 		{
// 			if (ch1 >= 'A' && ch1 <= 'Z')
// 				ch1 += 'a' - 'A';

// 			if (ch2 >= 'A' && ch2 <= 'Z')
// 				ch2 += 'a' - 'A';

// 			if (ch1 != ch2)
// 				return (int) ch1 - (int) ch2;
// 		}
// 		if (ch1 == 0)
// 			break;
// 	}
// 	return 0;
// }