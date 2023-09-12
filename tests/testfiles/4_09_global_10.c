int produit(int a, int b){
    return a * b;
}

int somme(int a, int b){
    return a + b;
}

int main() {
    int a = 5;
    if( a == 5) {
        int b = 12;
        if( b < 10) {
            a = produit(a ,b);
        } else {
            while( b < 15){
                a = somme(a, b);
                b = b + 1;
            }
        }
    }
    return a;
}