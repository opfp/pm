CREATE TABLE _index ( /* no confusion with vaults because _ not allowed as vaultname[0] */
    TB_NAME CHAR(15) PRIMARY KEY,
    MASTER_KEY BINARY(23),
    COMMKEY TINYINT,
    VIS TINYINT
);

CREATE TABLE _main_cmk (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);

CREATE TABLE _main_ukey (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    MASTER_KEY BINARY(23),
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);

/*
DROP TABLE test;

CREATE TABLE test (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    MASTER_KEY BINARY(23),
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);
*/
