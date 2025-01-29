#pragma once
#define _CRTDBG_MAP_ALLOC
// 파일 경로 , (줄 번호) , {메모리 할당 번호}
// , 블록 형식(표준, 클라이언트, CRT) , 메모리 주소 , 블록 크기(bytes)
#include <stdio.h> // Windows 체크 방법, mac : leaks 명령어 검색
#include <stdlib.h>
#include <Windows.h>
#include <crtdbg.h>

void checkLeak() {
    // 메모리 누수 내역 출력
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    // 프로그램 종료시 누수 체크
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
}
