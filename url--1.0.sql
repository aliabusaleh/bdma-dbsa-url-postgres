

\echo Use "CREATE EXTENSION url" to load this file. \quit
 
-- URL(varchar spec)
CREATE OR REPLACE FUNCTION url_in(cstring)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;


CREATE OR REPLACE FUNCTION url_in(cstring, cstring, int)
RETURNS url
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

-- varchar(URL spec)
CREATE OR REPLACE FUNCTION url_out(url)
RETURNS cstring
AS '$libdir/url'
LANGUAGE C IMMUTABLE STRICT;

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
	-- RECEIVE        = url_recv,
	-- SEND           = url_send,
	CATEGORY       = 'S'
);
COMMENT ON TYPE url IS 'url type  http://www.example.com<:port>/docs/resource1.html<#tag> where <> is optional';


-- -- casting methods 
-- CREATE OR REPLACE FUNCTION url(text)
-- RETURNS url
-- AS '$libdir/url', 'url_cast_from_text'
-- LANGUAGE C IMMUTABLE STRICT;

-- CREATE OR REPLACE FUNCTION text(url)
-- RETURNS text
-- AS '$libdir/url', 'url_cast_to_text'
-- LANGUAGE C IMMUTABLE STRICT;

