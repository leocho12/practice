#pragma once
#define _CRTDBG_MAP_ALLOC
// ���� ��� , (�� ��ȣ) , {�޸� �Ҵ� ��ȣ}
// , ��� ����(ǥ��, Ŭ���̾�Ʈ, CRT) , �޸� �ּ� , ��� ũ��(bytes)
#include <stdio.h> // Windows üũ ���, mac : leaks ��ɾ� �˻�
#include <stdlib.h>
#include <Windows.h>
#include <crtdbg.h>

void checkLeak() {
    // �޸� ���� ���� ���
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    // ���α׷� ����� ���� üũ
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}
