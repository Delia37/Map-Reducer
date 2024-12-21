#include <bits/stdc++.h>
#include <pthread.h>
#include <ctype.h>

using namespace std;

#define MAX_WORD_LEN 100
#define MAX_FILES 1000

struct thread_arg {
    vector<pair<string, int>> *fileInfo;
    int thread_id;
    int num_mappers;
    int num_reducers;
    int type;
    pthread_mutex_t *fileMutex;
    pthread_mutex_t *wordsMutex;
    map<string, set<int>> *allWords;
    pthread_barrier_t *barrier;
};

void check(string &word) {
    string ok;
    for (char c : word) {
        //daca cuvantul contine doar litere
        if (isalpha(c)) {
            ok += tolower(c); // litere mari -> mici
        }
    }
    word = ok;
}

void process_file(pair<string, int> &fileInfo, map<string, set<int>> &partialWords) {
    ifstream file(fileInfo.first);
    string word;

    // verific cuvintele citite
    while (file >> word) {
        check(word);
        if (!word.empty()) {
            // le pun pe cele valide in lista partiala
            partialWords[word].insert(fileInfo.second);
        }
    }
    file.close();
}

void merge_words(map<string, set<int>> &partialWords, map<string, set<int>> *allWords, pthread_mutex_t *wordsMutex) {
    pthread_mutex_lock(wordsMutex);
    for (const auto &entry : partialWords) {
        string word = entry.first; // cuvantul
        const set<int> &localFileIDs = entry.second; //FileIds din care face parte

        // Daca cuvantul nu exista in lista totala, il adaug
        if (allWords->find(word) == allWords->end()) {
            (*allWords)[word] = localFileIDs;
        } else {
            // Altfel, pun doar fileIds care nu se gasesc deja
            for (int fileID : localFileIDs) {
                (*allWords)[word].emplace(fileID);
            }
        }
    }
    pthread_mutex_unlock(wordsMutex);
}


void *func(void *arg) {
    thread_arg *t = (thread_arg *)arg;

    if (t->type == 0) { // Mapper
        pair<string, int> fileInfo;
        map<string, set<int>> partialWords;

        while (true) {
            // Lock pentru a nu prelua acelasi fisier simultan
            pthread_mutex_lock(t->fileMutex);
            if (t->fileInfo->empty()) {
                pthread_mutex_unlock(t->fileMutex);
                break;
            }
            fileInfo = t->fileInfo->back();
            t->fileInfo->pop_back();
            pthread_mutex_unlock(t->fileMutex);

            // Creez lista partiala a fiecarui mapper
            process_file(fileInfo, partialWords);
        }

        // O adaug la lista totala
        merge_words(partialWords, t->allWords, t->wordsMutex);
    }

    // Astept ca toti mapperii sa termine
    pthread_barrier_wait(t->barrier);

    if (t->type == 1) { // Reducer
        char startLetter = 'a' + (t->thread_id - t->num_mappers) * (26 / t->num_reducers);
        char endLetter = startLetter + (26 / t->num_reducers) - 1;

        // Sunt la ultimul reducer
        if (t->thread_id == t->num_mappers + t->num_reducers - 1) {
            endLetter = 'z'; // Ma asigur ca proceseaza literele pana la z
        }

        for (char ch = startLetter; ch <= endLetter; ++ch) {
            // cuvinte ce incep cu caracterul ch
            vector<pair<string, vector<int>>> chWords;

            for (auto &entry : *t->allWords) {
                if (tolower(entry.first[0]) == ch) {
                    vector<int> ids(entry.second.begin(), entry.second.end());
                    chWords.emplace_back(entry.first, ids);
                }
            }

            // Sortez cuvintele
            sort(chWords.begin(), chWords.end(), [](const auto &a, const auto &b) {
                if (a.second.size() == b.second.size()) return a.first < b.first; //alfabetic
                return a.second.size() > b.second.size(); // descrescator dupa numarul de fileIds
            });

            // Incep sa generez fisierele de output
            if (!chWords.empty()) {
                string filename = string(1, ch) + ".txt";
                ofstream out(filename);
                for (const auto &wordPair : chWords) {
                    out << wordPair.first << ":[";
                    for (size_t j = 0; j < wordPair.second.size(); ++j) {
                        if (j > 0) out << " ";
                        out << wordPair.second[j];
                    }
                    out << "]\n";
                }
                out.close();
            }
        }
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " <num_mappers> <num_reducers> <file_list>" << endl;
        return 1;
    }

    int num_mappers = stoi(argv[1]);
    int num_reducers = stoi(argv[2]);

    vector<pair<string, int>> fileInfo;
    ifstream file_list(argv[3]);
    string file_path;
    int file_count;
    file_list >> file_count;
    for (int i = 0; i < file_count; ++i) {
        file_list >> file_path;
        fileInfo.emplace_back(file_path, i + 1);
    }

    pthread_mutex_t fileMutex, wordsMutex;
    pthread_mutex_init(&fileMutex, NULL);
    pthread_mutex_init(&wordsMutex, NULL);

    map<string, set<int>> allWords;

    pthread_barrier_t barrier;
    pthread_barrier_init(&barrier, NULL, num_mappers + num_reducers);

    vector<thread_arg> args(num_mappers + num_reducers);
    vector<pthread_t> threads(num_mappers + num_reducers);

    for (int i = 0; i < num_mappers + num_reducers; ++i) {
        args[i] = {&fileInfo, i, num_mappers, num_reducers, i < num_mappers ? 0 : 1, &fileMutex, &wordsMutex, &allWords, &barrier};
        pthread_create(&threads[i], NULL, func, (void *)&args[i]);
    }

    for (auto &th : threads) {
        pthread_join(th, NULL);
    }

    pthread_mutex_destroy(&fileMutex);
    pthread_mutex_destroy(&wordsMutex);
    pthread_barrier_destroy(&barrier);

    return 0;
}
