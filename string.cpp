/*
 * CLCL
 *
 * String.c
 *
 * Copyright (C) 1996-2003 by Nakashima Tomoaki. All rights reserved.
 *		http://www.nakka.com/
 *		nakka@nakka.com
 */

/* Include Files */
#include <windows.h>
#include <tchar.h>

#include "Memory.h"

/* Define */
#define ToLower(c)		((c >= TEXT('A') && c <= TEXT('Z')) ? (c - TEXT('A') + TEXT('a')) : c)

/* Global Variables */

/* Local Function Prototypes */

/*
 * a2i - �����̕�����𐔒l(int)�ɕϊ�����
 */
int a2i(const char *str)
{
	int num = 0;
	int m = 1;

	if (*str == '-') {
		m = -1;
		str++;
	} else if (*str == '+') {
		str++;
	}

	for (; *str >= '0' && *str <= '9'; str++) {
		num = 10 * num + (*str - '0');
	}
	return (num * m);
}

/*
 * x2i - 16�i������𐔒l(int)�ɕϊ�����
 */
int x2i(const char *str)
{
	int ret = 0;
	int num;
	int i;

	if (str == NULL) {
		return 0;
	}

	if (*str == '0' && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
		str += 2;
	} else if (*str == 'x' || *str == 'X') {
		str++;
	}

	for (i = 0; i < 8 && *str != '\0'; i++, str++) {
		if (*str >= '0' && *str <= '9') {
			num = *str - '0';
		} else if (*str >= 'a' && *str <= 'f') {
			num = *str - 'a' + 10;
		} else if (*str >= 'A' && *str <= 'F') {
			num = *str - 'A' + 10;
		} else {
			break;
		}
		ret = ret * 16 + num;
	}
	return ret;
}

/*
 * tx2i - 16�i������(UNICODE)�𐔒l(int)�ɕϊ�����
 */
#ifdef UNICODE
int tx2i(const TCHAR *str)
{
	char *c_str;
	int ret = 0;

	if ((c_str = alloc_tchar_to_char(str)) != NULL) {
		ret = x2i(c_str);
		//mem_free(&((void*)c_str));
		mem_free((void**) &c_str);
	}
	return ret;
}
#endif

/*
 * str2hash - ������̃n�b�V���l���擾
 */
int str2hash(const TCHAR *str)
{
	int hash = 0;

	for (; *str != TEXT('\0'); str++) {
		if (*str != TEXT(' ')) {
			hash = ((hash << 1) + ToLower(*str));
		}
	}
	return hash;
}

/*
 * str_match - �Q�̕���������C���h�J�[�h(*)���g���Ĕ�r���s��
 */
BOOL str_match(const TCHAR *Ptn, const TCHAR *Str)
{
	switch (*Ptn) {
	case TEXT('\0'):
		return (*Str == TEXT('\0'));
	case TEXT('*'):
		return (str_match(Ptn + 1, Str) || (*Str != TEXT('\0')) && str_match(Ptn, Str + 1));
	case TEXT('?'):
		return (*Str != TEXT('\0')) && str_match(Ptn + 1, Str + 1);
	default:
		return (ToLower(*Ptn) == ToLower(*Str)) && str_match(Ptn + 1, Str + 1);
	}
}

/*
 * Trim - ������̑O��̋�, Tab����������
 */
BOOL Trim(TCHAR *buf)
{
	TCHAR *p, *r;

	// �O��̋󔒂��������|�C���^���擾
	for (p = buf; (*p == TEXT(' ') || *p == TEXT('\t')) && *p != TEXT('\0'); p++)
		;
	for (r = buf + lstrlen(buf) - 1; r > p && (*r == TEXT(' ') || *r == TEXT('\t')); r--)
		;
	*(r + 1) = TEXT('\0');

	// ���̕�����ɃR�s�[
	lstrcpy(buf, p);
	return TRUE;
}
/* End of source */