import shutil

def main():
    files = ["index.html", "App.js", "App.wasm"]
    srcPath = "./build_web_release"
    dstPath = "./"
    for file in files:
        shutil.copy2(srcPath + "/" + file, dstPath + "/" + file)
    return

if __name__ == "__main__":
    main()