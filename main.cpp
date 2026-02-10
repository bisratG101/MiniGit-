#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <map>
#include <set>
#include <algorithm>

namespace fs = std::filesystem;
using namespace std;

// simple hash is usefull to create a unique key based on the content of the file so each versions has 
// there own uniqe key and it is stored inside the obejct folder  
std::string simpleHash(const std::string& data) {
    std::hash<std::string> h;
    return std::to_string(h(data));
}
class MiniGit {
    fs::path repoPath, objectsPath, headPath, indexPath;

public:
    MiniGit(const std::string& base = ".") {
        repoPath   = fs::path(base) / ".minigit";
        objectsPath = repoPath / "objects";
        headPath    = repoPath / "HEAD";
        indexPath   = repoPath / "index";
        initRepo();
        std::cout << "Repo is initialized at: " << repoPath << endl;
    }

    void initRepo() {
        fs::create_directories(objectsPath); // to create a folder that holds the versions
        std::ofstream(headPath, std::ios::app);
        std::ofstream(indexPath, std::ios::trunc);
    }

    void add(const std::string& file) {//make a version ready to be commited 
        std::ifstream fin(file);
        if (!fin) { std::cerr << "File not found\n"; return; }// if file is empty return file not found 
        std::stringstream ss; // it creates string container
        ss << fin.rdbuf();// it takes everything from the file and copy it in ss container 
        std::string content = ss.str();// it copies it to the content container why use two containers 
        // because ss << fin.rdbuf(); is efficient and it dump it once otherwise we use while loop 
        std::string hash = simpleHash(content);
        fs::path obj = objectsPath / hash;// it creates the address of this version 
        std::ofstream(obj) << content;// it creates the file and store the content it with the label of hash

        std::ofstream idx(indexPath, std::ios::app);// it adds a line at the end
        idx << file << " " << hash << "\n";// the line is that is added it has file name and hash  which make it ready for commit 
        std::cout << "Added " << file << "\n";
    }

    void commit(const std::string& msg) {// msg is the message the wrriten by the user
        std::ifstream reader(indexPath);//open file for reading 
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(reader, line)){//read from the file and push it in to the vector
             lines.push_back(line);}

        std::string parent;// to store the previous commit hash
        std::ifstream headIn(headPath);
        std::getline(headIn, parent);

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());// to store the current time 
        std::ostringstream commitContent;
        commitContent << std::put_time(std::localtime(&now), "%F %T") << "|" << msg << "\n";// Add formatted current time
        commitContent << parent << "\n";
        for (auto& l : lines) {//it writes each line in the lines 
            commitContent << l << "\n";
        }

        std::string data = commitContent.str();
        std::string hash = simpleHash(data);//create a unique hash id for this commit
        std::ofstream(objectsPath / hash) << data;// save the commit data in file named by hash

        std::ofstream(headPath, std::ios::trunc) << hash;//it opens head and erases everything inside then writes the new commits hash 
        std::ofstream(indexPath, std::ios::trunc);// it erases indexfile to make it ready for the next 
        std::cout << "Committed: " << hash << "\n";
    }

    void log() {
        std::string cur;
        std::ifstream headIn(headPath);
        std::getline(headIn, cur);

        while (!cur.empty()) {
            fs::path commitPath = objectsPath / cur;
            std::ifstream in(commitPath);
            if (!in) break;
            

            std::string metaLine, parent;
            std::getline(in, metaLine);  // line 1: date|message
            std::getline(in, parent);    // line 2: parent hash

            std::cout << "Commit " << cur << " | " << metaLine << "\n";

            cur = parent; // move to parent
        }
    }

    void branch(const std::string& branchName) {
        // creates the "refs" directory in the .minigit folder
        fs::path refsPath = repoPath / "refs";
        
        // makes sure refs folder is available
        fs::create_directories(refsPath);

        // creates a path for a new branch file
        fs::path branchFile = refsPath / branchName;

        // read the current commit hash from the HEAD file
        std::ifstream headFile(headPath);
        std::string currentCommit;
        std::getline(headFile, currentCommit);
        headFile.close();

        // check whether a commit in HEAD exists to create a branch
        if (currentCommit.empty()) {
            std::cerr << "Error: NO commit exist in HEAD.\n";
            return;
        }

        // write the current commit hash into the new branch file
        std::ofstream branchOut(branchFile);
        branchOut << currentCommit;
        branchOut.close();

        // tell the user that the branch is successfully created
        std::cout << "The Branch '" << branchName << "' created, pointing to " << currentCommit << "\n";
    }

    void checkoutBranch(const std::string& branchName) {
        // creates the way to the branch file under .minigit/refs/
        fs::path branchPath = repoPath / "refs" / branchName;

        // check whether the branch file exists
        if (!fs::exists(branchPath)) {
            std::cerr << "The Branch '" << branchName << "DOES NOT EXIST.\n";
            return;
        }
    
        // Read commithash that is stored in the branch file
        std::ifstream branchIn(branchPath);
        std::string commitHash;
        std::getline(branchIn, commitHash);
        branchIn.close();

        // locate the related commit file using the hash commit
        fs::path commitFile = objectsPath / commitHash;
        if (!fs::exists(commitFile)) {
            std::cerr << "Commit file does not exist for branch '" << branchName << "'.\n";
            return;
        }

        // make the commit file read its contents
        std::ifstream commitIn(commitFile);
        std::string line;
        std::getline(commitIn, line);  // Skip timestamp or commit message
        std::getline(commitIn, line);  // Skip parent commit hash

        // Read and restore each tracked file
        // each line shows a file and its hash
        while (std::getline(commitIn, line)) {
            std::istringstream iss(line);
            std::string filename, fileHash;
            iss >> filename >> fileHash;

            fs::path objectFile = objectsPath / fileHash;
            if (!fs::exists(objectFile)) {
                std::cerr << "Missing object for " << filename << "\n";
                continue;
            }

            // Restore the file by copying its content from the object file
            std::ifstream inFile(objectFile);
            std::ofstream outFile(filename);
            outFile << inFile.rdbuf();
            std::cout << "Restored: " << filename << "\n";
        }

        // Update HEAD to point to the recent commit from checkout branch
        std::ofstream(headPath, std::ios::trunc) << commitHash;
        std::cout << "Switched to branch '" << branchName << "'\n";
    }


    void merge(const std::string& branchName) {
    fs::path branchPath = repoPath / "refs" / branchName;

    if (!fs::exists(branchPath)) {
        std::cerr << "Branch '" << branchName << "' does not exist.\n";
        return;
    }

    // Get commit hashes
    std::string currentCommitHash, targetCommitHash;
    std::ifstream headIn(headPath); std::getline(headIn, currentCommitHash); headIn.close();
    std::ifstream branchIn(branchPath); std::getline(branchIn, targetCommitHash); branchIn.close();

    if (currentCommitHash.empty() || targetCommitHash.empty()) {
        std::cerr << "Missing commit hash.\n";
        return;
    }

    // === Find LCA ===
    std::set<std::string> visited;
    std::string lca = "";

    // Traverse current branch
    std::string cur = currentCommitHash;
    while (!cur.empty()) {
        visited.insert(cur);
        std::ifstream commitFile(objectsPath / cur);
        std::string skip, parent;
        std::getline(commitFile, skip); // timestamp
        std::getline(commitFile, parent); // parent
        cur = parent;
    }


    // Traverse target branch and find common ancestor
    cur = targetCommitHash;
    while (!cur.empty()) {
        if (visited.count(cur)) {
            lca = cur;
            break;
        }
        std::ifstream commitFile(objectsPath / cur);
        std::string skip, parent;
        std::getline(commitFile, skip);
        std::getline(commitFile, parent);
        cur = parent;
    }

    if (lca.empty()) {
        std::cerr << "No common ancestor found.\n";
        return;
    }

    // === Helper: load file list from commit ===
    auto loadFiles = [&](const std::string& commitHash) {
        std::map<std::string, std::string> fileMap;
        std::ifstream file(objectsPath / commitHash);
        std::string line;
        std::getline(file, line); // skip timestamp
        std::getline(file, line); // skip parent
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string fname, fhash;
            iss >> fname >> fhash;
            fileMap[fname] = fhash;
        }
        return fileMap;
    };

    auto baseFiles = loadFiles(lca);
    auto currFiles = loadFiles(currentCommitHash);
    auto otherFiles = loadFiles(targetCommitHash);

    std::vector<std::pair<std::string, std::string>> filesToMerge;

    for (const auto& [filename, otherHash] : otherFiles) {
        std::string baseHash = baseFiles.count(filename) ? baseFiles[filename] : "";
        std::string currHash = currFiles.count(filename) ? currFiles[filename] : "";

        if (currHash == otherHash || otherHash == baseHash) {
            continue; // no change or same as base
        }

        if (currHash != baseHash && currHash != "" && baseHash != "" && currHash != otherHash) {
            std::cout << "CONFLICT: both modified " << filename << "\n";
            continue;
        }

        // Safe to merge
        fs::path objectFile = objectsPath / otherHash;
        if (!fs::exists(objectFile)) {
            std::cerr << "Missing object for " << filename << "\n";
            continue;
        }

        std::ifstream inFile(objectFile);
        std::ofstream outFile(filename);
        outFile << inFile.rdbuf();
        std::cout << "Merged: " << filename << "\n";

        filesToMerge.emplace_back(filename, otherHash);
    }

    // Update index with merged files
    std::ofstream idx(indexPath, std::ios::app);
    for (const auto& [filename, fileHash] : filesToMerge)
        idx << filename << " " << fileHash << "\n";

    // Make commit for merged state
    commit("Merged branch " + branchName);
}


    void diff(const std::string& commit1, const std::string& commit2) {
    // Helper: Load all files and their hashes from a given commit
    auto loadFiles = [&](const std::string& commitHash) {
        std::map<std::string, std::string> fileMap;
        std::ifstream file(objectsPath / commitHash);
        std::string line;
        std::getline(file, line); // skip timestamp
        std::getline(file, line); // skip parent
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string fname, fhash;
            iss >> fname >> fhash;
            fileMap[fname] = fhash;
        }
        return fileMap;
    };

    auto getLinesFromObject = [&](const std::string& hash) {
        std::vector<std::string> lines;
        std::ifstream file(objectsPath / hash);
        std::string line;
        while (std::getline(file, line))
            lines.push_back(line);
        return lines;
    };

    auto files1 = loadFiles(commit1);
    auto files2 = loadFiles(commit2);

    std::set<std::string> allFiles;
    for (const auto& [f, _] : files1) allFiles.insert(f);
    for (const auto& [f, _] : files2) allFiles.insert(f);

    for (const auto& file : allFiles) {
        std::cout << "\n--- File: " << file << " ---\n";

        auto it1 = files1.find(file), it2 = files2.find(file);

        if (it1 == files1.end()) {
            std::cout << "+ File added in " << commit2 << "\n";
            continue;
        }
        if (it2 == files2.end()) {
            std::cout << "- File deleted in " << commit2 << "\n";
            continue;
        }

        auto lines1 = getLinesFromObject(it1->second);
        auto lines2 = getLinesFromObject(it2->second);

        size_t maxLen = std::max(lines1.size(), lines2.size());
        for (size_t i = 0; i < maxLen; ++i) {
            std::string l1 = i < lines1.size() ? lines1[i] : "";
            std::string l2 = i < lines2.size() ? lines2[i] : "";

            if (l1 != l2) {
                std::cout << "- " << l1 << "\n";
                std::cout << "+ " << l2 << "\n";
            }
        }
    }
    }

};


int main() {
    MiniGit mg;
    string input;

    cout << "Welcome to MiniGit CLI!\nType commands like: init, add <file>, commit <msg>, log, branch <name>, checkout <branch>, merge <branch>, diff <commit1> <commit2>, exit\n";

    while (true) {
        cout << "\nMiniGit> ";
        getline(cin, input);

        stringstream ss(input);
        string cmd, arg;
        ss >> cmd;

        if (cmd == "exit") break;

        else if (cmd == "init") {
            mg.initRepo();
        }

        else if (cmd == "add") {
            ss >> arg;
            if (arg.empty()) cout << "Usage: add <filename>\n";
            else mg.add(arg);
        }

        else if (cmd == "commit") {
            getline(ss, arg); // get the full message
            if (arg.empty()) cout << "Usage: commit <message>\n";
            else {
                // Remove leading space from message
                arg = arg.substr(arg.find_first_not_of(" "));
                mg.commit(arg);
            }
        }

        else if (cmd == "log") {
            mg.log();
        }

        else if (cmd == "branch") {
            ss >> arg;
            if (arg.empty()) cout << "Usage: branch <branch-name>\n";
            else mg.branch(arg);
        }

        else if (cmd == "checkout") {
            ss >> arg;
            if (arg.empty()) cout << "Usage: checkout <branch-name>\n";
            else mg.checkoutBranch(arg);
        }


        else if (cmd == "merge") {
            ss >> arg;
            if (arg.empty()) cout << "Usage: merge <branch-name>\n";
            else mg.merge(arg);
        }


        else if (cmd == "diff") {
            std::string commit1, commit2;
            ss >> commit1 >> commit2;
            if (commit1.empty() || commit2.empty()) {
                cout << "Usage: diff <commit1> <commit2>\n";
            } else {
                mg.diff(commit1, commit2); // Show line-by-line differences
            }
        }


        else {
            cout << "Unknown command: " << cmd << "\n";
        }
    }

    return 0;
}
