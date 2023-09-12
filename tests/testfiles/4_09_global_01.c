int fibo(int n){
    int val_return;
    if ( n < 1 ){
        val_return = 0;
    }else{
        if(n==1){
            val_return = 1;
        }else{
            val_return = fibo(n-1) + fibo(n-2);
        }
    }
    return val_return;
}
int main(){
    return 2*fibo(20);
}