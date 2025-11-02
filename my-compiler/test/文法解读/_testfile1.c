#include <stdio.h>

const int con0 = 1;
const int con1 = 2, con2 = 3;
const int conarr[3] = {1, 2, 3};

int varundef;
int var0 = 1;
int var1 = 2, var2 = 3;
int vararrundef[3];
int vararr[3] = {1, 2, 3};

void p0() {
    printf("f0\n");
    return;
}
void p1(int a) {
    printf("%d\n", a);
    return;
}
void p2(int a, int b) {
    printf("%d %d\n", a, b);
    return;
}
int a1() {
    return 1;
}
int a2(int a) {
    return a;
}
int a3(int a, int b) {
    return a + b;
}
int arr0(int a[]) {
    return a[0];
}
int main() {
    int x = 0;
    int y[2] = {1, 2};
    static int sta0 = 1;
    static int sta1 = 2, sta2 = 3;
    static int starr[3] = {1, 2, 3};
    printf("22377272\n");
    x = var1 + var2;
    x = var1 - var2;
    x = var1 * var2;
    x = var1 / var2;
    x = var1 % var2;
    x = (var1 + var2) * (var1 - var2);
    x = a1();
    x = a2(x);
    x = a3(x, x);
    y[0] = +1;
    y[0] = -1;
    {
        ;
    }
    {
        
    }
    {
        p0();
    }
    if(x) {
        ;
    } else {
        ;
    }
    if(x > 0) x = 0;
    if(x < 0) x = 0;
    if(x >= 0) x = 0;
    if(x <= 0) x = 0;
    if(x == 0) x = 0;
    if(x != 0) x = 0;
    if(x && 1) x = 0;
    if(x || 1) x = 0;
    if(!x) x = 0;
    if(x || 1 || 1 && 1 && !x || 1) x = 0;
    for(x = 0; x < 0; x = x + 1) {
        continue;
    }
    for(x = 0; x < 0;) {
        ;
    }
    for(x = 0; ;x = x + 1) {
        break;
    }
    for(;x < 0; x = x + 1) {
        break;
    }
    for(;;) {
        break;
    }
    for(x = 0, y[0] = 0;;) {
        break;
    }
    for(;x < 0;) {
        break;
    }
    for(;;x = x + 1) {
        break;
    }
    return 0;
}