#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <linux/netlink.h>

#include "manager.h"

int skfd;
struct sockaddr_nl saddr, daddr;

char vtcmno[8];
int vtcmno_num = 1;
int new_rec_flag = 1;

int rcv_from_kernel(user_msg_info *u_info, int *len)
{
	int ret = 0;
	memset(u_info, 0, sizeof(user_msg_info));
	ret = recvfrom(skfd, u_info, sizeof(user_msg_info), 0, (struct sockaddr *)&daddr, len);
	return ret;
}

int send_to_kernel(char *vtcmno)
{
	int ret = 0;
	struct nlmsghdr *nlh = NULL;
	nlh = malloc(NLMSG_SPACE(MAX_PLOAD));
	memset(nlh, 0, sizeof(struct nlmsghdr));
	nlh->nlmsg_len = NLMSG_SPACE(MAX_PLOAD);
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_type = 0;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = saddr.nl_pid;
	memcpy(NLMSG_DATA(nlh), vtcmno, sizeof(vtcmno));
	ret = sendto(skfd, nlh, nlh->nlmsg_len, 0, (struct sockaddr *)&daddr, sizeof(struct sockaddr_nl));
	return ret;
}

static int get_vtcmno(void *not_use, int col_cnt, char **row_txt, char **col_name)
{
	// there is only one result can be found
	// col_name[0] -> "vtcmno"
	// row_txt[0] -> vtcmno / NULL
	if (row_txt[0]) {
		new_rec_flag = 0;
		strcpy(vtcmno, row_txt[0]);
		printf("    return recorder: %s\n", vtcmno);
	}
	return 0;
}

static int get_vtcmno_num(void *not_use, int col_cnt, char **row_txt, char **col_name)
{
	vtcmno_num++;
	return 0;
}

int main(int argc, char const *argv[])
{
	int ret = 0;
	sqlite3 *conn = NULL;
	char *err_msg = NULL;
	char sqlstr[1024];
	user_msg_info u_info;
	int len = 0;
	// setup netlink
	skfd = socket(AF_NETLINK, SOCK_RAW, NETLINK_UNIT);
	if (skfd < 0) {
		perror("create netlink socket error: ");
		return skfd;
	}
	memset(&saddr, 0, sizeof(saddr));
	saddr.nl_family = AF_NETLINK;
	saddr.nl_pid = USER_PORT;
	saddr.nl_groups = 0;
	ret = bind(skfd, (struct sockaddr *)&saddr, sizeof(saddr));
	if (ret) {
		perror("bind error: ");
		close(skfd);
		return ret;
	}
	memset(&daddr, 0, sizeof(daddr));
	daddr.nl_family = AF_NETLINK;
	daddr.nl_pid = 0;
	daddr.nl_groups = 0;
	// open db and try to create table
	ret = sqlite3_open(
		DB_PATH,
		&conn
	);
	if (ret != SQLITE_OK) {
		perror("fatal error - cannot open sqlite database: ");
		close(skfd);
		return ret;
	}
	ret = sqlite3_exec(
		conn,
		CREATE_TABLE,
		NULL,
		NULL,
		&err_msg
	);
	if (ret != SQLITE_OK) {
		// already created table, just alarm error msg
		printf("%s\n", err_msg);
		sqlite3_free(err_msg);
		err_msg = NULL;
	}
	memset(&sqlstr[0], 0, sizeof(sqlstr));
	memset(&vtcmno[0], 0, sizeof(vtcmno));
	// get vtcmno_num
	printf("scan database to find out next vtcmno...\n");
	ret = sqlite3_exec(
		conn,
		ALL_DATA,
		get_vtcmno_num,
		NULL,
		&err_msg
	);
	if (ret != SQLITE_OK) {
		printf("%s\n", err_msg);
		goto out;
	}
	printf("=> next vtcmno is: vtcm%d\n", vtcmno_num);
	// main loop
	printf("*** start manager main loop ***\n");
	while (1) {
		// receive uuid from kernel
		printf("\n[wait msg from kernel...]\n");
		ret = rcv_from_kernel(&u_info, &len);
		if (ret < 0) {
			perror("rcv msg from kernel error: ");
			goto out;
		}
		printf("    uuid: %s\n", u_info.msg);
		sprintf(sqlstr, QUERRY_UUID, u_info.msg);
		ret = sqlite3_exec(
			conn,
			sqlstr,
			get_vtcmno,
			(void *)(&vtcmno),
			&err_msg
		);
		if (ret != SQLITE_OK) {
			printf("%s\n", err_msg);
			goto out;
		}
		// alloc vtcmno if necessary
		if (new_rec_flag) {
			sprintf(vtcmno, VTCMNO, vtcmno_num++);
			printf("    new recorder: %s\n", vtcmno);
		}
		// return vtcmno to kernel
		ret = send_to_kernel(vtcmno);
		if (ret <= 0) {
			perror("send message to kernel error: ");
			goto out;
		}
		// make a new record if necessary
		if (new_rec_flag) {
			memset(&sqlstr[0], 0, sizeof(sqlstr));
			sprintf(sqlstr, INSERT_RECORD, u_info.msg, vtcmno);
			ret = sqlite3_exec(
				conn,
				sqlstr,
				NULL,
				NULL,
				&err_msg
			);
			if (ret != SQLITE_OK) {
				printf("%s\n", err_msg);
				goto out;
			}
		}
		// clean
		memset(&sqlstr[0], 0, sizeof(sqlstr));
		memset(&vtcmno[0], 0, sizeof(vtcmno));
		new_rec_flag = 1;
	}
out:
	close(skfd);
	if (conn)
		sqlite3_close(conn);
	if (err_msg)
		sqlite3_free(err_msg);
	return ret;
}
