#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"MemoryLeak.h"
//윷놀이 만들기
void p();//게임 시작
void p1();//1P가 윷 던지기
void p2();//2P가 윷 던지기
void wasd();//현재 위치에서 상하좌우로 말 움직이기
void ijkl();//현재위치에서 대각선으로 말 움직이기

int main() {
	p();
	srand((unsigned int)time(NULL));
	printf("윷놀이 판 출력\n");
    for (int i = 1; i <= 1; i++) {
        //윗변
        for (int j = 1; j <= 6; j++)printf("■ ");
        printf("\n");
    }
    for (int i = 1; i <= 6 - 1 * 2; i++) {
        //좌변
        for (int j = 1; j <= 1; j++) printf("■");
        //대각선 칸
        for (int j = 0; j < 5; j++) {
            for (int q = 0; q < 5-j; q++) {
                printf("  ");
            }
            printf("■");
        }
        //우변
        for (int j = 1; j <= 1; j++) printf("■");
        printf("\n");
    }
    for (int i = 1; i <= 1; i++) {
        //밑변
        for (int j = 1; j <= 6; j++)printf("■ ");
        printf("\n");
    }
    printf("\n");

	return 0;
}

void p()
{
	printf("윷놀이 게임입니다.\n\n게임을 시작합니다.\n\n1P와 2P가 번갈아 가며 윷을 던져주세요.\n\n");
}

//void p1()
//{
//	printf("1P가 윷을 던질 차례입니다.\n");
//	printf("스페이스바를 눌러 윷을 던져주세요\n");
//	char input;
//	int roll;
//	scanf("%s", input);
//	if (input == " ") {
//		roll = rand() % 6 + 1;
//		switch (roll) {
//		case 1:
//			printf("□ □ □ ■  //---도\n"); 
//			break;
//		case 2:
//			printf("□ □ ■ ■  //---개\n");
//			break;
//		case 3:
//			printf("□ ■ ■ ■  //---걸\n");
//			break;
//		case 4:
//			printf("■ ■ ■ ■  //---윷\n");
//			break;
//		case 5:
//			printf("□ □ □ □  //---모\n");
//			break;
//		}
//
//	}
//}
//
//void p2()
//{
//}
