int greater_lower_equal(int test, int refValue){
    int result = 0;
    if(test < refValue){
        result = -1;
    } else {
        if(test > refValue){
            result = 1;
        } else {
            result = 0;
        }
    }
    return result;
}

int main() {
    int a = 5;
    int b = greater_lower_equal(a, 4);
    int c = greater_lower_equal(a, 5);
    int d = greater_lower_equal(a, 6);
    return b+c+d;
}