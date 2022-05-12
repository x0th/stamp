Cat = Object^;
Cat barks = true;
Cat make_right = fn() { if (this.barks) { this barks = false; } }
Cat.make_right();
Cat.barks;
