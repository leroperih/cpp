#include <windows.h>
#include <iostream>

int main() {
    // O HKL (Handle de Layout de Teclado) para "English (United States) - US" é 00000409
    // Usamos LoadKeyboardLayout para obter o ponteiro HKL correspondente a essa string
    HKL hklToDelete = LoadKeyboardLayoutA("00000409", KLF_NOTELLSHELL);

    if (hklToDelete == NULL) {
        std::cerr << "[ERRO] Nao foi possivel encontrar ou carregar o identificador do layout US." << std::endl;
        return 1;
    }

    // Descarrega (deleta da sessao atual) o layout de teclado especificado
    if (UnloadKeyboardLayout(hklToDelete)) {
        std::cout << "[SUCESSO] O layout de teclado 'English (United States) - US' foi removido com sucesso!" << std::endl;
    }
    else {
        std::cerr << "[ERRO] Falha ao remover o layout de teclado. Codigo de erro: " << GetLastError() << std::endl;
    }

    system("pause");

    return 0;
}