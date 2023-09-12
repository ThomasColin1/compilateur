int soustraction(int a, int b){
    a = a - b;
    return a;
}

int produit(int a, int b){
    a = soustraction(a, b);
    return a * b;
}

int division(int a, int b){
    return a / b;
}

int somme(int a, int b, int c, int d, int e, int f){
    a = produit(a, b);
    b = division(f, c);
    return a+b+c+d+e+f;
}

int main(){
    int s = 0;
    s = somme(1, 2, 3, 4, 5, 6);
    return s;    
}