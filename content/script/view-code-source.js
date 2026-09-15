
// 1. Linhas de cima continuam aqui
const main_Element = document.querySelector('div.two-sides-structure-content');
const which_iframe = "main-iframe";

// 2. Em vez de DOMContentLoaded, usamos uma IIFE Assíncrona:
(async () => {
    console.log("A função começou a rodar com sucesso!");

    // Pega os parâmetros do link (?folder=win&project=02)
    const urlParams = new URLSearchParams(window.location.search);
    const tipoPasta = urlParams.get('folder');   // "win" ou "dx"
    const numProjeto = urlParams.get('project'); // "02", "03", etc.

    if (!tipoPasta || !numProjeto) {
        console.error("Parâmetros 'folder' ou 'project' ausentes na URL.");
        return;
    }

    // Caminho base para a pasta do sistema (ex: content/code-source/win)
    const caminhoBaseSistema = `content/code-source/${tipoPasta}`;

    try {
        // Busca o arquivo all-projects.json dentro da pasta raiz do sistema (win ou dx)
        const resposta = await fetch(`${caminhoBaseSistema}/all-projects.json`);
        const dados = await resposta.json();
        
        // Procura no array de 'projects' o objeto correspondente ao projeto da URL
        const projetoEncontrado = dados.projects.find(p => p.folder === numProjeto);

        if (!projetoEncontrado) {
            console.error(`Projeto com a pasta '${numProjeto}' não foi encontrado no arquivo all-projects.json.`);
            return;
        }

        // Filtra as chaves do objeto que começam com 'file-' (ex: file-01)
        const chavesDeArquivos = Object.keys(projetoEncontrado).filter(chave => chave.startsWith('file-'));

        // Limpa o main para reconstruir com os iframes
        main_Element.innerHTML = "";

        chavesDeArquivos.forEach((chave) => {
            const nomeArquivoOriginal = projetoEncontrado[chave];

            // Monta o caminho final: content/code-source/win/program-02/main-source.cpp
            const templateIframe = `
                <h2 class="file-title"> ${nomeArquivoOriginal} </h2>
                <div class="iframe-source-code">
                    <iframe src="${caminhoBaseSistema}/program-${numProjeto}/${nomeArquivoOriginal}" frameborder="0" class="show-code"></iframe>
                </div>
            `;

            main_Element.innerHTML += templateIframe;
        });

    } catch (erro) {
        console.error("Erro ao buscar ou processar o arquivo all-projects.json:", erro);
    }
})();