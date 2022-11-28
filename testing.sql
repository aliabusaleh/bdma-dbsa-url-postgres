
create EXTENSION url;

create table test(arg url);

insert into
    test
values
(url_in('https://www.google.com'));

insert into
    test
select
    url_in(arg, '/test')
from
    test;

insert into
    test
values
(url_in('https', 'www.yahoo.com', 32, 'abc.png'));

insert into
    test
values
(url_in('http', 'www.ahmad.com', 'test.gif'));

insert into
    test
values
(url_in('https://www.google.com?q=a&c=4'));
insert into
    test
values
(url_in('https://abc:xyz@www.yes.com#njcdncnd'));

insert into
    test
values
(url_in('https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd'));

select
    getPort(arg) as port,
    getDefaultPort(arg) as defaultport,
    getHost(arg) as host,
    getProtocol(arg) as protocol,
    getQuery(arg) as query,
    getUserInfo(arg) as UserInfo,
    getRef(arg) as reference,
    getPath(arg) as path,
    getFile(arg) as file, 
    getAuthority(arg) as authority,
    to_string(arg) as res,
    arg
from
    test;


drop table test;
drop EXTENSION url;