import sys
import time
sys.path.insert(0, '.')

code = """
function calc(a,b, c) {
  return data+' '+(a+b+c);
}
a = 1;
b = 2;
c = 3;

a = {'lista': ['roberta','serena', 'alessandro', calc(a,b,c), 17, 30, 26, ['unbit'], 17.30], 'foo':'bar'};
"""

code2 = """
data['values'];
data;
"""

code3 = """
Object.keys(data)
"""

code4 = """
data['values'][5][0].toUpperCase();
"""

import pydrone

data = {'values': ['foobar', (17.0,30.0), 'test', (1,2,3,4,5), str(time.time()), ['one', 'two']], 'foo':'bar'}

for i in range(1,100):
    print(pydrone.js(code, data))
    print(data, '=', pydrone.js(code2, data))
    print(pydrone.js(code3, data))
    print(pydrone.js(code4, data))
    try:
        print("refcnt:",sys.gettotalrefcount())
        with open('/proc/self/stat') as f:
            stat = f.read().split()
            print("vsz",stat[22],"rss",stat[23])
    except:
        pass
