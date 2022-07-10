import sys
import os

inDir = "./src"
outDir = "./src/embedded"

def as_cpp_string(content):
    return "R\"xxx({cn})xxx\"".format(cn = content);

def as_cpp_key_value(key, value):
    return "{{ {k}, {v} }}".format(k = key, v = value)

def main():
    files = [f for f in os.listdir(inDir) if os.path.isfile(inDir + "/" + f)]
    shaders = [f for f in files if os.path.splitext(f)[1] == ".glsl"]
    files_embed_out = outDir + "/" + "textfiles.cpp"
    with open(files_embed_out, mode = "w") as output:
        keypairs = []
        for shader in shaders:
            with open(inDir + "/" + shader) as input:
                #output.write(input.red)
                key = as_cpp_string(shader)
                value = as_cpp_string(input.read())
                kp = as_cpp_key_value(key, value)
                keypairs.append(kp)

        res = ",\n".join(keypairs)
        output.write(res)


if __name__ == "__main__":
    main()