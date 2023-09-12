int main() {
    int len = 21;
    int i = 3;
    int t1 = 0;
    int t2 = 1;
    int nextTerm = t1 + t2;
    while (i < len + 1 ){
        t1 = t2;
        t2 = nextTerm;
        nextTerm = t1 + t2;
        i = i +1;
    }

    return nextTerm;
}