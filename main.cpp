#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include "json.hpp"

using json = nlohmann::json;
using namespace std;

string ReadFile(const string& path)
{
    ifstream f(path);
    if (!f.is_open()) return "";
    stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

string LibToPath(const string& group, const string& artifact, const string& ver)
{
    string g = group;
    replace(g.begin(), g.end(), '.', '/');
    return g + "/" + artifact + "/" + ver + "/" + artifact + "-" + ver + ".jar";
}

int main()
{
    string gameRoot = R"(D:\PCL\.minecraft)";
    string verName = "1.21.10";
    string verJsonPath = gameRoot + R"(\versions\)" + verName + R"(\)" + verName + ".json";
    // 修正：文件夹名是 1.21.10-natives
    string nativeDir = gameRoot + R"(\versions\)" + verName + R"(\1.21.10-natives)";

    string raw = ReadFile(verJsonPath);
    if (raw.empty())
    {
        cout << "无法读取 version.json！\n";
        system("pause");
        return 1;
    }

    json j = json::parse(raw);
    string mainClass = j["mainClass"];
    cout << "✅ 主类：" << mainClass << "\n";

    vector<string> cpList;
    string selfJar = gameRoot + R"(\versions\)" + verName + R"(\)" + verName + ".jar";
    cpList.push_back(selfJar);

    auto& libs = j["libraries"];
    for (auto& lib : libs)
    {
        bool skip = false;
        if (lib.contains("rules"))
        {
            for (auto& rule : lib["rules"])
            {
                if (rule.contains("os") && rule["os"]["name"] != "windows")
                {
                    skip = true;
                }
            }
        }
        if (skip) continue;

        string name = lib["name"];
        size_t p1 = name.find(':');
        size_t p2 = name.find(':', p1 + 1);
        string group = name.substr(0, p1);
        string art = name.substr(p1 + 1, p2 - p1 - 1);
        string ver = name.substr(p2 + 1);

        string relPath = LibToPath(group, art, ver);
        string fullPath = gameRoot + R"(\libraries\)" + relPath;
        cpList.push_back(fullPath);
    }

    string classpath;
    for (int i = 0; i < cpList.size(); i++)
    {
        if (i > 0) classpath += ";";
        classpath += cpList[i];
    }

    cout << "\n📦 Classpath 总长度：" << classpath.size() << "\n";
    cout << "🔹 依赖数量（含本体）：" << cpList.size() << "\n";
    cout << "📂 Native库目录：" << nativeDir << "\n";

    string javaCmd = "java -Djava.library.path=\"" + nativeDir + "\" -Xmx4G -Xms2G -cp \"" + classpath + "\" " + mainClass
        + " --accessToken 0 --version " + verName + " "
        + "--gameDir \"" + gameRoot + "\" "
        + "--assetsDir \"" + gameRoot + R"(\assets)" + "\" "
        + "--assetIndex " + verName;

    wstring wcmd(javaCmd.begin(), javaCmd.end());
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi{};

    if (CreateProcessW(nullptr, wcmd.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        cout << "\n🎮 Minecraft进程已启动\n";
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        cout << "\n👋 游戏退出\n";
    }
    else
    {
        cout << "❌ 启动失败，错误码：" << GetLastError() << "\n";
    }

    system("pause");
    return 0;
}