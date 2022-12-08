# DBMA: DBSA Project 2 (Postgres Extension: URL)
This project is done under the BDMA first semester at Université Libre de Bruxelles


## <b>Overview</b>
Create a PostgreSQL (Postgres) extension that provides a Uniform Resource Locator URL data type. A URL represents a
pointer to a "resource" on the World Wide Web. A resource can be something as simple as a file or a directory, or it can be a
reference to a more complicated object, such as a query to a database or to a search engine. A URL has a structure, which is
standardized by W3C.

The goal of this project is to create an equivalent type to java.net.URL class in Postgres. More information about the java
class can be found [here](https://docs.oracle.com/javase/8/docs/api/java/net/URL.html)


This project is done by: </br>

* <b>Ali AbuSaleh
* Muhammad Rizwan Khalid
* Ahmad 
* Prashant Gupta</b></br>

Under supervision of professor <b>Mahmoud Sakr</b>

## <b>Prerequisites</b>
* [liburiparser library](https://pypi.org/project/uniparser/)
* [PostgreSQL](https://www.digitalocean.com/community/tutorials/how-to-install-postgresql-on-ubuntu-20-04-quickstart)
* Python 
# Notes & Descriptions
* <b>Constractor</b> methods are
  * ```'www.google.com'::url```
  * ```url_in(URL context, varchar spec)```
  * ```url_in(varchar protocol, varchar host, varchar file)```
  * ```url_in(varchar protocol, varchar host, int port, varchar file)```
* <b>Casting</b>
  * ```Cast ( text as url)``` 
  * ``` 'text'::url```
* <b>Operators </b>
  * operator (<b>=</b>) : <b>compare Hosts Only</b>.
* <b>Other methods</b>

  * ```sameHost(url, url)```:  <pre> Compare only hosts of the URLs</pre>
  * ```sameFile(url, url)```:   <pre> Compare whole URLs <b>without fragment</b></pre>
  * ```equals(url, url)```:     <pre> Compare whole URLs <b>including fragment</b></pre>
  * ```getPort(url)```: <pre> Get the port fir a Given URL.</pre> 
  * ```getDefaultPort(url)```: <pre> Get Default Port for a given URL.</pre>
  * ```getHost(url)```: <pre> Get the Host for a given URL.</pre>
  * ```getProtocol(url)```: <pre> Get the protocol for a given URL.</pre>
  * ```getQuery(url)``` <pre> Get the query for a given URL.</pre>
  * ```getUserInfo(url)``` <pre> Get UserInfo for a given URL.</pre>
  * ```getRef(url)``` <pre> Get the reference for a given URL.</pre>
  * ```getPath(url)``` <pre> Get the path for a given URL.</pre>
  * ```getFile(url)``` <pre> Get the file for a given URL. </pre>
  * ```getAuthority(url)``` <pre> Get the authority for a given URL.</pre>
  * ```to_string(url)``` <pre> cast URL to string.</pre>
# <b>How to run the code </b>
### Build the extension
```
$ pip install uniparser 
$ git clone https://github.com/aliabusaleh/bdma-dbsa-url-postgres.git
$ cd bdma-dbsa-url-postgres
$ sudo make  
$ sudo make install
```
### <b>how to use the extension in PostgreSQL</b>
<b>*Login to Postgres using your creds*</b>
```
postgres=# create extension url;
CREATE EXTENSION
```
## <b>Testing </b>

<b>using testing.sql</b>
### How to test the extension 
```
postgres=# create table test(arg url);
CREATE TABLE
postgres=# 
postgres=# insert into test(arg) values (url_in('https', 'www.yahoo.com', 32, 'abc.png'));
INSERT 0 1
postgres=# insert into test select url_in(arg, '/test') from test;
INSERT 0 1
postgres=# 
postgres=# insert into test values ('https://www.google.com?q=a&c=4');
INSERT 0 1
postgres=# insert into test values ('https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd');
INSERT 0 1
postgres=# insert into test values('www.abc.com');
INSERT 0 1
postgres=# insert into test values('www.abc.com/33');
INSERT 0 1
postgres=# insert into test values('www.google.com');
INSERT 0 1
postgres=# insert into test values('www.google.com/path1/path2');
INSERT 0 1
postgres=# insert into test values('www.google.com/path1/path2/file.html');
INSERT 0 1
postgres=# insert into test values('www.google.com/path1/path2/file.html#2');
INSERT 0 1
postgres=# insert into test values('www.google.com/path1/path2/file.html#2');
INSERT 0 1
postgres=# insert into test values('www.google.com/path1/path2/path3/file.html#2');
INSERT 0 1
postgres=# 
postgres=# create index arg_idx on test(arg);
CREATE INDEX
postgres=# select * from test ;
                       arg                        
--------------------------------------------------
 https://www.yahoo.com:32/abc.png
 https://www.yahoo.com:32/abc.png/test
 https://www.google.com?q=a&c=4
 https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd
 www.abc.com
 www.abc.com/33
 www.google.com
 www.google.com/path1/path2
 www.google.com/path1/path2/file.html
 www.google.com/path1/path2/file.html#2
 www.google.com/path1/path2/file.html#2
 www.google.com/path1/path2/path3/file.html#2
(12 rows)

postgres=# set enable_seqscan=False; -- disable sequantal scan
SET
postgres=# 
postgres=# select * from test where arg = 'www.abc.com'; -- check host only
      arg       
----------------
 www.abc.com
 www.abc.com/33
(2 rows)

postgres=# select * from test where arg = 'www.abc.com/22'; -- check host only
      arg       
----------------
 www.abc.com
 www.abc.com/33
(2 rows)

postgres=# 
postgres=# select * from test where sameHost(arg, 'www.abc.com'); -- check only host
      arg       
----------------
 www.abc.com
 www.abc.com/33
(2 rows)

postgres=# select * from test where sameHost(arg, 'www.abc.com/newpath/'); -- check only host
      arg       
----------------
 www.abc.com
 www.abc.com/33
(2 rows)

postgres=# select * from test where sameHost(arg, 'www.google.com/newpath/pathtest'); -- check only host
                     arg                      
----------------------------------------------
 https://www.google.com?q=a&c=4
 www.google.com
 www.google.com/path1/path2
 www.google.com/path1/path2/file.html
 www.google.com/path1/path2/file.html#2
 www.google.com/path1/path2/file.html#2
 www.google.com/path1/path2/path3/file.html#2
(7 rows)

postgres=# select * from test where sameHost(arg, 'www.yes.com'); -- check only host
                       arg                        
--------------------------------------------------
 https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd
(1 row)

postgres=# 
postgres=# select * from test where sameFile(arg, 'www.google.com/path1/path2/home.html'); -- check file
 arg 
-----
(0 rows)

postgres=# select * from test where sameFile(arg, 'www.google.com/path1/path2/file.html#4'); -- file only ignore fragment
                  arg                   
----------------------------------------
 www.google.com/path1/path2/file.html
 www.google.com/path1/path2/file.html#2
 www.google.com/path1/path2/file.html#2
(3 rows)

postgres=# select * from test where sameFile(arg, 'www.google.com/path1/path2/path3/file.html#66'); -- file only ignore fragment
                     arg                      
----------------------------------------------
 www.google.com/path1/path2/path3/file.html#2
(1 row)

postgres=# select * from test where sameFile(arg, 'https://abc:xyz@www.yes.com/abc/xyz?s=3#22newfrag'); -- file only ignore fragment ( case diff fragment )
                       arg                        
--------------------------------------------------
 https://abc:xyz@www.yes.com/abc/xyz?s=3#njcdncnd
(1 row)

postgres=# select * from test where sameFile(arg, 'https://abc:xyz@www.yes.com/abc/xyz?s=newquery'); -- -- file only ignore fragment ( case diff file )
 arg 
-----
(0 rows)

postgres=# 
postgres=# 
postgres=# select * from test where equals(arg, 'www.abc.com/22'); -- check all url
 arg 
-----
(0 rows)

postgres=# select * from test where equals(arg, 'www.google.com/path1'); -- check all url
 arg 
-----
(0 rows)

postgres=# select * from test where equals(arg, 'www.abc.com/33'); -- check all url
      arg       
----------------
 www.abc.com/33
(1 row)

postgres=# 

```

# Run extension using Docker

## Installation

To easily install and use the extension without worrying about platform. Please follow below steps

1. Install docker in your system. Please follow the below link to install docker in windows and mac respectively

[Docker Windows](https://docs.docker.com/desktop/install/windows-install/)

[Docker Mac](https://docs.docker.com/desktop/install/mac-install/)

[Docker Linux](https://docs.docker.com/desktop/install/linux-install/)


2. Move to the directory of source code of extension

3. Run Below command to start docker container

```bash
  docker-compose up -d
```

4. After start of docker container. Enter the docker container.
```bash
  docker exec -it postgre_ext bash
```

5. In docker container, start the postgres process.
```bash
  pg_ctlcluster 14 main start
```

6. Build the extension with `make` command
```bash
  make
  make install
```

7. Setup extension in postgres
```bash
  sudo -i -u postgres
  psql postgres
```

8. Inside postgres install the extension
```bash
  create extension url;
```

Now extension is installed and ready to be used.


## <b>Question?</b>
<b>Feel free to contact us for more questions regarding this extension</b>
