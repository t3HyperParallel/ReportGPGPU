
#pragma once

// lenti.cu 関数名
// ここでマクロを定義しておくと名前の変更が楽になる

#define NAME_TO_STRING(name) (#name)

#define COLOR24BITTO32BIT color24bitTo32bit
#define COLOR24BITTO32BIT_NAME NAME_TO_STRING(COLOR24BITTO32BIT)

#define WRITELENTI writeLenti
#define WRITELENTI_NAME NAME_TO_STRING(WRITELENTI)