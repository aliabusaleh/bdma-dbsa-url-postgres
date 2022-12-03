drop table test;
drop extension url;
\q
psql;

create extension url;
create table test(arg url);

insert into test(arg) values (url_in('https', 'www.yahoo.com', 32, 'abc.png'));
insert into test select url_in(arg, '/test') from test;

insert into test values ('https://www.google.com?q=a&c=4');
insert into test values ('https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd');
insert into test values('www.abc.com');
insert into test values('www.abc.com/33');
insert into test values('www.google.com');
insert into test values('www.google.com/path1/path2');
insert into test values('www.google.com/path1/path2/file.html');
insert into test values('www.google.com/path1/path2/file.html#2');
insert into test values('www.google.com/path1/path2/file.html#2');
insert into test values('www.google.com/path1/path2/path3/file.html#2');


create index arg_idx on test(arg);
select * from test ;
set enable_seqscan=False; -- disable sequantal scan

select * from test where arg = 'www.abc.com'; -- check host only
select * from test where arg = 'www.abc.com/22'; -- check host only

select * from test where sameHost(arg, 'www.abc.com'); -- check only host
select * from test where sameHost(arg, 'www.abc.com/newpath/'); -- check only host
select * from test where sameHost(arg, 'www.google.com/newpath/pathtest'); -- check only host
select * from test where sameHost(arg, 'www.yes.com'); -- check only host

select * from test where sameFile(arg, 'www.google.com/path1/path2/home.html'); -- check file
select * from test where sameFile(arg, 'www.google.com/path1/path2/file.html#4'); -- file only ignore fragment
select * from test where sameFile(arg, 'www.google.com/path1/path2/path3/file.html#66'); -- file only ignore fragment
select * from test where sameFile(arg, 'https://abc:xyz@www.yes.com/abc/xyz?s=3#22newfrag'); -- file only ignore fragment ( case diff fragment )
select * from test where sameFile(arg, 'https://abc:xyz@www.yes.com/abc/xyz?s=newquery'); -- -- file only ignore fragment ( case diff file )


select * from test where equals(arg, 'www.abc.com/22'); -- check all url
select * from test where equals(arg, 'www.google.com/path1'); -- check all url
select * from test where equals(arg, 'www.abc.com/33'); -- check all url


-- select
--     getPort(arg) as port,
--     getDefaultPort(arg) as defaultport,
--     getHost(arg) as host,
--     getProtocol(arg) as protocol,
--     getQuery(arg) as query,
--     getUserInfo(arg) as UserInfo,
--     getRef(arg) as reference,
--     getPath(arg) as path,
--     getFile(arg) as file, 
--     getAuthority(arg) as authority,
--     to_string(arg) as res,
--     arg
-- from
--     test;



--sudo make PG_CONFIG=/usr/bin/pg_config
-- sudo make install
