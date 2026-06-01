double add_double(double a, double b) {
    double c = a + b;
    return c;
}

int add_int(int a, int b) {
    int c = a + b;
    return c;
}

int main() {
    short s = 3;
    int x = 10;
    float f = 3.14;
    double d = 2.0;

    double y = add_double(f, d);
    int z = add_int(s, x);

    if (y > z) {
        return 1;
    } else {
        return 0;
    }
}