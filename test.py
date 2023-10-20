import time
import sys
from my_py_profile import MyProfile

def example_1():
    print(f"Execute: {sys._getframe().f_code.co_name}")
    p = MyProfile("example_1")
    time.sleep(0.5)

def example_2():
    print(f"Execute: {sys._getframe().f_code.co_name}")
    p = MyProfile("example_2", sleep=0.2)
    a=[]
    for i in range(10000):
        a.append(i)
    time.sleep(0.2)
    del p
    del a

def example_3():
    print(f"Execute: {sys._getframe().f_code.co_name}")
    p = MyProfile("example_3", sleep=0.4, localvar='test local variable scope.')
    time.sleep(0.2)
    if 0 < 20:
        p1 = MyProfile("example_3_1", sleep=0.1)
        time.sleep(0.1)
        del p1 # Need release, and end 'p1' scope.
    p2 = MyProfile("example_3_2", sleep=0.1)
    time.sleep(0.1)
    # Release order: p1->p->p2, this order trigger diagram mess.
    # So release manually.
    del p2
    del p
    # New release order: p1->pt2->p

def main():
    example_1()
    example_2()
    example_3()

if __name__ == "__main__":
    main()
