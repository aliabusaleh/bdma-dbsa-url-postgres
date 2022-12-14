\echo Use "CREATE EXTENSION url" to load this file.\quit -- URL(varchar spec)
-- Usage insert into test values(url_in('https://www.google.com'));
CREATE OR REPLACE FUNCTION url_in(cstring)
RETURNS url
AS '$libdir/url','url_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION url_in(text)
RETURNS url
AS '$libdir/url','url_in'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION url_in(cstring, cstring, integer, cstring)
RETURNS url
AS '$libdir/url', 'url_in_part_two'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION url_in(cstring, cstring, cstring)
RETURNS url
AS '$libdir/url', 'url_in_part_three'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION url_in(url, cstring)
RETURNS url
AS '$libdir/url', 'url_in_part_four'
LANGUAGE C IMMUTABLE STRICT;
-- varchar(URL spec)
CREATE
OR REPLACE FUNCTION url_out(url) RETURNS cstring AS '$libdir/url' LANGUAGE C IMMUTABLE STRICT;

-- Commenting recv and send code for now. Will check with professor
--- operations over wire (API)
-- CREATE OR REPLACE FUNCTION url_recv(internal)
-- RETURNS url
-- AS '$libdir/url'
-- LANGUAGE C IMMUTABLE STRICT;
-- CREATE OR REPLACE FUNCTION url_send(url)
-- RETURNS bytea
-- AS '$libdir/url'
-- LANGUAGE C IMMUTABLE STRICT;
-- create the new type 
CREATE TYPE url (
        INPUT = url_in,
        OUTPUT = url_out,
        -- 	RECEIVE        = url_recv,
        -- 	SEND           = url_send,
        CATEGORY = 'S'
);

COMMENT ON TYPE url IS 'url type  http://www.example.com<:port>/docs/resource1.html<#tag> where <> is optional';

-- casting methods 
CREATE OR REPLACE FUNCTION url_cast(text)
RETURNS url
AS '$libdir/url', 'url_cast_from_text'
LANGUAGE C IMMUTABLE STRICT;

CREATE CAST (url AS text) WITH INOUT AS ASSIGNMENT;
CREATE CAST (text AS url) WITH FUNCTION url_in(text) AS ASSIGNMENT ;

-- Get host function
CREATE FUNCTION getHost(url) RETURNS text AS '$libdir/url',
'get_host' LANGUAGE C IMMUTABLE STRICT;

-- Get port function
CREATE FUNCTION getPort(url) RETURNS integer AS '$libdir/url',
'get_port' LANGUAGE C IMMUTABLE STRICT;

-- Get port function
CREATE FUNCTION getDefaultPort(url) RETURNS integer AS '$libdir/url',
'get_default_port' LANGUAGE C IMMUTABLE STRICT;

-- Get protocol function
CREATE FUNCTION getProtocol(url) RETURNS text AS '$libdir/url',
'get_protocol' LANGUAGE C IMMUTABLE STRICT;

-- Get protocol function
CREATE FUNCTION getQuery(url) RETURNS text AS '$libdir/url',
'get_query' LANGUAGE C IMMUTABLE STRICT;

-- Get UserInfo function
CREATE FUNCTION getUserInfo(url) RETURNS text AS '$libdir/url',
'get_user_info' LANGUAGE C IMMUTABLE STRICT;

-- Get Ref function
CREATE FUNCTION getRef(url) RETURNS text AS '$libdir/url',
'get_ref' LANGUAGE C IMMUTABLE STRICT;


-- Get Path 
CREATE FUNCTION getPath(url) RETURNS text AS '$libdir/url',
'get_path' LANGUAGE C IMMUTABLE STRICT;

-- Get File 
CREATE FUNCTION getFile(url) RETURNS text AS '$libdir/url',
'get_file' LANGUAGE C IMMUTABLE STRICT;

-- Get Authority
CREATE FUNCTION getAuthority(url) RETURNS text AS '$libdir/url',
'get_authority' LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION to_string(url) RETURNS text AS '$libdir/url',
'get_string' LANGUAGE C IMMUTABLE STRICT;

-- Same Host
CREATE FUNCTION sameHostInternal(url, url) RETURNS boolean AS '$libdir/url',
'same_host' LANGUAGE C IMMUTABLE STRICT;


-- Same URL
CREATE FUNCTION sameUrlInternal(url, url) RETURNS boolean AS '$libdir/url',
'same_url' LANGUAGE C IMMUTABLE STRICT;

-- Same File
CREATE FUNCTION sameFileInternal(url, url) RETURNS boolean AS '$libdir/url',
'same_file' LANGUAGE C IMMUTABLE STRICT;

-- equals
CREATE FUNCTION equalsInternal(url, url) RETURNS boolean AS '$libdir/url',
'same_url' LANGUAGE C IMMUTABLE STRICT;

-- Not equals
CREATE FUNCTION NotequalsInternal(url, url) RETURNS boolean AS
'select NOT(sameFile($1, $2)) OR NOT(equalsInternal($1, $2))' LANGUAGE SQL IMMUTABLE STRICT;


---------- Btree ----

-- equals
CREATE FUNCTION equals(url, url) RETURNS boolean AS  
'select sameFile($1, $2) AND equalsInternal($1, $2)' LANGUAGE SQL IMMUTABLE STRICT;

-- Btree cmp 
CREATE FUNCTION Btreecmp(url, url) RETURNS integer AS '$libdir/url',
'btree_cmp' LANGUAGE C IMMUTABLE STRICT;

-- Same File
CREATE FUNCTION sameFile(url, url) RETURNS boolean AS 
'select $1 = $2 AND  sameFileInternal($1, $2) ' LANGUAGE SQL IMMUTABLE STRICT;

CREATE FUNCTION sameHost(url, url) RETURNS boolean AS 
'select $1 = $2 ' LANGUAGE SQL IMMUTABLE STRICT;


-- Functions for operators
CREATE FUNCTION url_abs_rt(url, url) RETURNS boolean AS '$libdir/url',
'url_abs_rt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_abs_lt(url, url) RETURNS boolean AS '$libdir/url',
'url_abs_lt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_rt(url, url) RETURNS boolean AS '$libdir/url',
'url_rt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_lt(url, url) RETURNS boolean AS '$libdir/url',
'url_lt' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION url_cmp_btree(url, url) RETURNS integer AS '$libdir/url',
'url_cmp_internal' LANGUAGE C IMMUTABLE STRICT;


CREATE OPERATOR < (
        leftarg = url,
        rightarg = url,
        procedure = url_abs_rt,
        commutator = >,
        negator = >=,
        HASHES, MERGES
);

CREATE OPERATOR > (
        leftarg = url,
        rightarg = url,
        procedure = url_abs_lt,
        commutator = <,
        negator = <=,
        HASHES, MERGES
);


CREATE OPERATOR <= (
        leftarg = url,
        rightarg = url,
        procedure = url_lt,
        commutator = >=,
        negator = >,
        HASHES, MERGES
);


CREATE OPERATOR >= (
        leftarg = url,
        rightarg = url,
        procedure = url_rt,
        commutator = <=,
        negator = <,
        HASHES, MERGES
);

CREATE OPERATOR = (
        leftarg = url,
        rightarg = url,
        procedure = sameHostInternal,
        HASHES, MERGES
);

CREATE OPERATOR <> (
        leftarg = url,
        rightarg = url,
        procedure = NotequalsInternal,
        COMMUTATOR = '<>',
	NEGATOR = '=',
	RESTRICT = neqsel,
	JOIN = neqjoinsel
);

CREATE OPERATOR CLASS btree_url_ops
DEFAULT FOR TYPE url USING btree
AS
        OPERATOR        1       <  ,
        OPERATOR        2       <= ,
        OPERATOR        3       =  ,
        OPERATOR        4       >= ,
        OPERATOR        5       >  ,
        FUNCTION        1       Btreecmp(url, url);
