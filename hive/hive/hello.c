#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include"MemoryLeak.h"
//������ �����
void p();//���� ����
void p1();//1P�� �� ������
void p2();//2P�� �� ������
void wasd();//���� ��ġ���� �����¿�� �� �����̱�
void ijkl();//������ġ���� �밢������ �� �����̱�

int main() {
	p();
	srand((unsigned int)time(NULL));
	printf("������ �� ���\n");
    for (int i = 1; i <= 1; i++) {
        //����
        for (int j = 1; j <= 6; j++)printf("�� ");
        printf("\n");
    }
    for (int i = 1; i <= 6 - 1 * 2; i++) {
        //�º�
        for (int j = 1; j <= 1; j++) printf("��");
        //�밢�� ĭ
        for (int j = 0; j < 5; j++) {
            for (int q = 0; q < 5-j; q++) {
                printf("  ");
            }
            printf("��");
        }
        //�캯
        for (int j = 1; j <= 1; j++) printf("��");
        printf("\n");
    }
    for (int i = 1; i <= 1; i++) {
        //�غ�
        for (int j = 1; j <= 6; j++)printf("�� ");
        printf("\n");
    }
    printf("\n");

	return 0;
}

void p()
{
	printf("������ �����Դϴ�.\n\n������ �����մϴ�.\n\n1P�� 2P�� ������ ���� ���� �����ּ���.\n\n");
}

//void p1()
//{
//	printf("1P�� ���� ���� �����Դϴ�.\n");
//	printf("�����̽��ٸ� ���� ���� �����ּ���\n");
//	char input;
//	int roll;
//	scanf("%s", input);
//	if (input == " ") {
//		roll = rand() % 6 + 1;
//		switch (roll) {
//		case 1:
//			printf("�� �� �� ��  //---��\n"); 
//			break;
//		case 2:
//			printf("�� �� �� ��  //---��\n");
//			break;
//		case 3:
//			printf("�� �� �� ��  //---��\n");
//			break;
//		case 4:
//			printf("�� �� �� ��  //---��\n");
//			break;
//		case 5:
//			printf("�� �� �� ��  //---��\n");
//			break;
//		}
//
//	}
//}
//
//void p2()
//{
//}
