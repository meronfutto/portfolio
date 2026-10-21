#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <climits>

using namespace std;

int main(){
    ifstream fin("matrix.txt");
    if (!fin.is_open()) {
        cout << "File not found\n";
        return 1;
    }

    const int N=50;
    string a[N][N];
    long long W[N][N];
    long long D[N];
    int P[N];
    bool used[N];
    const long long NEG_INF = -1000000000000000LL;
    const long long POS_INF = 1000000000000000LL;
    const int START = 11;

    for(int i = 0; i < N; i++){
        for(int j = 0; j < N; j++){
            fin >> a[i][j];
        }
    }

    for(int i=0;i<N;i++){
        for(int j=0;j<N;j++){
            if(a[i][j] == "*"){
                W[i][j] = NEG_INF;
            }
            else{
                W[i][j]=stoll(a[i][j]);
            }
        }
    }

    for(int i = 0; i<N; i++){
        W[i][i]=POS_INF;
    }

    for(int i = 0; i<N; i++){
        D[i]=W[START][i];
    }

    for (int i = 0; i < N; i++) {
        P[i] = START;
    }
    

    for (int i = 0; i < N; i++) {
        used[i] = false;
    }
    used[START] = true;
    int iter=1;

    cout << iter << "\n";
    cout << "D: ";
    for (int i = 0; i < N; i++) {
        if (D[i] == POS_INF) {
            cout << "* ";
        } else if (D[i] == NEG_INF) {
            cout << "- ";
        } else {
            cout << D[i] << " ";
        }
    }
    cout << "\n";

    cout << "P: ";
    for (int i = 0; i < N; i++) {
        cout << P[i] + 1 << " ";
    }
    cout << "\n";


    for(int step=2;step<=N;step++){
        int best = -1;
        for(int i=0;i<N;i++){
            if(!used[i]){
                if (best==-1 || D[i] > D[best]){
                    best=i;
                }
            }
        }
        if (best==-1 || D[best] == NEG_INF) break;

        used[best]=true;
        bool changed = false;

        for(int i = 0; i<N; i++){
            if(!used[i]){
                long long candidate = min(D[best], W[best][i]);

                if(candidate>D[i]){
                    D[i] = candidate;
                    P[i] = best;
                    changed = true;
                }
            }
        }
        if (changed) {
            iter++;
            cout << iter << "\n";
            cout << "D: ";
            for (int i = 0; i < N; i++) {
                if (D[i] == POS_INF) {
                    cout << "* ";
                } else if (D[i] == NEG_INF) {
                    cout << "- ";
                } else {
                    cout << D[i] << " ";
                }
            }
            cout << "\n";

            cout << "P: ";
            for (int i = 0; i < N; i++) {
                cout << P[i] + 1 << " ";
            }
            cout << "\n";
        }
    }

    
    return 0;
}