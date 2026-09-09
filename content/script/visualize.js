
    const main_Element = document.querySelector("main");
    const which_iframe = "main-iframe";

    const iframe_Element = `
        <div class="code">
            <iframe src="" frameborder="0" class="show-code" id="${which_iframe}"></iframe>
        </div>`;






    async function listarTxtDaSubpasta(nomeSubpasta)
    {
        try
        {
            // Pede ao usuário para selecionar a pasta raiz (ex: a pasta "ALL")
            const handleRaiz = await window.showDirectoryPicker();
            
            // Acessa a subpasta específica baseada no seu parâmetro 'which_folder'
            const handleSubpasta = await handleRaiz.getDirectoryHandle(nomeSubpasta);
            
            let listaArquivosTxt = [];

            // Itera apenas sobre os arquivos que terminam com .txt
            for await ( const entrada of handleSubpasta.values() )
            {
                if ( entrada.kind === 'file' && entrada.name.toLowerCase().endsWith('.txt') )
                {
                    listaArquivosTxt.push(entrada.name);
                }
            }

            console.log(`Sucesso! Encontrados ${listaArquivosTxt.length} arquivos .txt:`, listaArquivosTxt);
            return listaArquivosTxt;

        } catch (erro) {
            console.error("Erro ao acessar a pasta:", erro);
        }
    }
    listarTxtDaSubpasta();






document.addEventListener("DOMContentLoaded", async function () {
    // 1. Pega o número da pasta enviado pela URL
    const urlParams = new URLSearchParams(window.location.search);
    const pastaAlvo = urlParams.get('folder'); // Ex: "02"

    if (!pastaAlvo) {
        console.error("Nenhuma pasta foi informada na URL.");
        return;
    }

    try {
        // 2. Lê o seu arquivo de configuração JSON (ajuste o caminho se necessário)
        const resposta = await fetch('code-source/windows/all-projects.json');
        const dados = await resposta.json();
        
        // 3. Procura no JSON o objeto que tem a pasta correspondente
        const projetoEncontrado = dados.projects.find(p => p.folder === pastaAlvo);

        if (!projetoEncontrado) {
            console.error(`Pasta "${pastaAlvo}" não foi encontrada no all-projects.json.`);
            return;
        }

        // 4. Descobre quais arquivos existem olhando as chaves do objeto (ex: "file-01", "file-02")
        // Filtramos para pegar apenas as chaves que começam com "file-"
        const chavesDeArquivos = Object.keys(projetoEncontrado).filter(chave => chave.startsWith('file-'));

        console.log(`Sucesso! Encontrados ${chavesDeArquivos.length} arquivos na pasta ${pastaAlvo}.`);

        // 5. Limpa ou prepara o elemento principal
        main_Element.innerHTML = "";

        // 6. Faz um loop por todos os arquivos encontrados no JSON para esta pasta
        chavesDeArquivos.forEach((chave, index) => {
            const nomeArquivoOriginal = projetoEncontrado[chave]; // Ex: "main-source.cpp"
            
            // Descobre o nome sem a extensão para buscar o arquivo .txt correspondente
            // Ex: "main-source.cpp" vira "main-source"
            const nomeSemExtensao = nomeArquivoOriginal.substring(0, nomeArquivoOriginal.lastIndexOf('.'));

            // Cria um ID único para cada iframe (ex: main-iframe-0, main-iframe-1)
            const idIframeUnico = `main-iframe-${index}`;

            // Cria a estrutura HTML dinamicamente para cada arquivo
            const templateIframe = `
                <div class="code-block" style="margin-bottom: 30px;">
                    <h2 class="file-title"> ${nomeArquivoOriginal} </h2>
                    <div class="code">
                        <iframe src="ALL/program-${pastaAlvo}/${nomeSemExtensao}/${nomeSemExtensao}.txt" frameborder="0" class="show-code" id="${idIframeUnico}"></iframe>
                    </div>
                </div>
            `;

            // Adiciona o bloco na tela
            main_Element.innerHTML += templateIframe;
        });

    } catch (erro) {
        console.error("Erro ao carregar ou processar os dados do projeto:", erro);
    }
});