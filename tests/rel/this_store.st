Dog = Object^;
Dog barks = false;
Dog make_right = fn() { this barks = true; }
Dog.make_right();
Dog.barks
