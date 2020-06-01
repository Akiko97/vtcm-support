#ifndef VTCM_MANAGER
#define VTCM_MANAGER

#define NETLINK_UNIT 29
#define USER_PORT 101
#define MSG_LEN 256
#define MAX_PLOAD 256

#define DB_PATH "./vtcm_db.sqlite3"
#define CREATE_TABLE "create table vms_vtcm(uuid text,vtcmno text)"
#define QUERRY_UUID "select vtcmno from vms_vtcm where uuid='%s'"
#define INSERT_RECORD "insert into vms_vtcm(uuid,vtcmno) values('%s','%s')"
#define ALL_DATA "select * from vms_vtcm"

#define VTCMNO "vtcm%d"

typedef struct _user_msg_info {
	struct nlmsghdr hdr;
	char msg[MSG_LEN];
} user_msg_info;

int rcv_from_kernel(user_msg_info *, int *);
int send_to_kernel(char *);
static int get_vtcmno(void *, int, char **, char **);
static int get_vtcmno_num(void *, int, char **, char **);

#endif