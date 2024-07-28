import subprocess

def build():
    print("Building...")
    subprocess.call(["cmake", "-B", "build", ".", "-DCMAKE_PROJECT_TOP_LEVEL_INCLUDES=conan_provider.cmake", "-DCMAKE_BUILD_TYPE=Release"])

if __name__ == "__main__":
    build()
