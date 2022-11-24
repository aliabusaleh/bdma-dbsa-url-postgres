

\echo Use "CREATE EXTENSION url" to load this file. \quit
 
-- URL(varchar spec)
-- Usage insert into test values(url_in('https://www.google.com'));
CREATE OR REPLACE FUNCTION url_in(cstring)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

-- URL(varchar protocol, varchar host, int port, varchar file)
-- Usage insert into test values(url_in('https','www.yahoo.com',32,'abc.png'));
CREATE OR REPLACE FUNCTION url_in(cstring, cstring, integer, cstring)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

-- URL(varchar protocol, varchar host, varchar file)
-- Usage insert into test values(url_in('http','www.ahmad.com','test.gif'));
CREATE OR REPLACE FUNCTION url_in(cstring, cstring, cstring)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

-- URL(URL context, varchar spec)
CREATE OR REPLACE FUNCTION url_in(url, cstring)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;




-- varchar(URL spec)
CREATE OR REPLACE FUNCTION url_out(url)
RETURNS cstring
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;


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
	INPUT          = url_in,
	OUTPUT         = url_out,
-- 	RECEIVE        = url_recv,
-- 	SEND           = url_send,
	CATEGORY       = 'S'
);
COMMENT ON TYPE url IS 'url type  http://www.example.com<:port>/docs/resource1.html<#tag> where <> is optional';


-- casting methods 
CREATE OR REPLACE FUNCTION url(text)
RETURNS url
AS '$libdir/url', 'url_cast_from_text'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION text(url)
RETURNS text
AS '$libdir/url', 'url_cast_to_text'
LANGUAGE C IMMUTABLE STRICT;

-- Btree
CREATE FUNCTION url_cmp(ntext, ntext)
RETURNS integer
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS btree_url_ops
DEFAULT FOR TYPE url USING btree
AS
        OPERATOR        1       <  ,
        OPERATOR        2       <= ,
        OPERATOR        3       =  ,
        OPERATOR        4       >= ,
        OPERATOR        5       >  ,
        FUNCTION        1       url_cmp(url, url);
