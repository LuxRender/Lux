# This file is part of LuxRender.
# 
# converts all .crf files in current directory for embedding into lux

def convert_crf(filedata):
    import re
    
    func_expr = re.compile('(?P<funcname>\w\\S+)\\s+graph.+\\s+I\\s*=\\s*(?P<I>.+$)\\s+B\\s*=\\s*(?P<B>.+$)', re.MULTILINE)
    channel_expr = re.compile("^(?P<name>.*)(?P<channel>Red|Green|Blue)$", re.IGNORECASE)
    float_expr = re.compile('\s*(-?\\d*\\.?\\d+(?:[eE][-+]?\\d+)?)')
    
    fh = open('crfs_h.txt', 'a')
    fc = open('crfs_cpp.txt', 'a')
    fn = open('crfs_names.txt', 'a')

    ns = '%NAME%\n'
    cs = 'else if (name == "%NAME%") {\n'
    
    for m in func_expr.finditer(filedata):
        
        name = m.group('funcname').replace('-', '_')
        name = name[0].capitalize() + name[1:]

        channel = channel_expr.match(name)
        if channel != None:
            name = channel.group('name')
            channel = channel.group('channel').capitalize()
        else: 
            channel = "Red"

        for a in ('I', 'B'):
            s = 'static float ' + name + channel + '_' + a + '[] = { '
            s += float_expr.sub('\g<1>f, ', m.group(a))
            s = s[:-2] + ' };\n'
            fh.write(s)
            
        cs += '\tconst size_t n%CHANNEL% = sizeof(%NAME%%CHANNEL%_I) / sizeof(%NAME%%CHANNEL%_I[0]);\n'
        cs += '\t%CHANNEL%I.assign(%NAME%%CHANNEL%_I, %NAME%%CHANNEL%_I + n%CHANNEL%);\n'
        cs += '\t%CHANNEL%B.assign(%NAME%%CHANNEL%_B, %NAME%%CHANNEL%_B + n%CHANNEL%);\n'
        cs = cs.replace('%CHANNEL%', channel)
        cs = cs.replace('%NAME%', name)
        
        ns = ns.replace('%NAME%', name)
            
    fh.write('\n')

    cs += "}\n"
    fc.write(cs)

    fn.write(ns)


if __name__ == "__main__":
    import os
    crfs = [fn for fn in os.listdir('.') if os.path.splitext(fn)[1] == '.crf']

    for n in ('crfs_h.txt', 'crfs_cpp.txt', 'crfs_names.txt'):
        try:
            os.remove(n)
        except:
            pass

    for crf in crfs:
        f = open(crf, 'r')
        data = f.read()

        convert_crf(data)
    
    print "Converted %d CRFs" % (len(crfs))
