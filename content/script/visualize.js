// 1. Linhas de cima (que já funcionam fora) continuam aqui
const main_Element = document.querySelector('div.two-sides-structure-content');
const which_iframe = "main-iframe";

const iframe_Element = `
    <div class="code">
        <iframe src="" frameborder="0" class="show-code" id="${which_iframe}"></iframe>
    </div>`;

// 2. Em vez de DOMContentLoaded, usamos uma IIFE Assíncrona:
(async () => {
    console.log("A função começou a rodar com sucesso!");

    // Pega os parâmetros do link (?folder=dx&project=004)
    const urlParams = new URLSearchParams(window.location.search);
    const tipoPasta = urlParams.get('folder');   // "dx" ou "win"
    const numProjeto = urlParams.get('project'); // "004"

    if (!tipoPasta || !numProjeto) {
        console.error("Parâmetros 'folder' ou 'project' ausentes na URL.");
        return;
    }

    // Caminho para a pasta baseado na sua nova estrutura
    const caminhoPasta = `content/code-source/${tipoPasta}/program-${numProjeto}`;

    try {
        // Busca o arquivo files.json dentro da pasta
        const resposta = await fetch(`${caminhoPasta}/all-projects.json`);
        const dados = await resposta.json();
        
        const chavesDeArquivos = Object.keys(dados).filter(chave => chave.startsWith('file-'));

        // Limpa o main e reconstrói com os dados do JSON
        main_Element.innerHTML = "";

        chavesDeArquivos.forEach((chave, index) => {
            const nomeArquivoOriginal = dados[chave];

            const templateIframe = `
                <div class="code-block" style="margin-bottom: 30px;">
                    <h2 class="file-title"> ${nomeArquivoOriginal} </h2>
                    <div class="code">
                        <iframe src="${caminhoPasta}/${nomeArquivoOriginal}" frameborder="0" class="show-code"></iframe>
                    </div>
                </div>
            `;

            main_Element.innerHTML += templateIframe;
        });

    } catch (erro) {
        console.error("Erro ao buscar ou processar o arquivo files.json:", erro);
    }
})();