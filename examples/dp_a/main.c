// https://atcoder.jp/contests/dp/tasks/dp_a
#include <stdio.h>

int dp[100010];
int N;
int h[100010];

int abs(int a) {
    if (a < 0)
        return -a;
    return a;
}

void chmax(int *a, int b) {
    if (*a < b)
        *a = b;
}
void chmin(int *a, int b) {
    if (*a > b)
        *a = b;
}

int main() {
    scanf("%d", &N);
    for (int i = 0; i < N; i++) {
        scanf("%d", &h[i]);
    }

    dp[0] = 0;
    for (int i = 1; i <= N; i++) {
        dp[i] = 1000000000;
    }
    for (int i = 0; i < N; i++) {
        chmin(&dp[i + 2], dp[i] + abs(h[i + 2] - h[i]));
        chmin(&dp[i + 1], dp[i] + abs(h[i + 1] - h[i]));
    }
    printf("%d\n", dp[N-1]);
    return 0;
}
