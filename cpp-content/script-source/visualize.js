
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






    document.addEventListener("DOMContentLoaded", function ()
    {

        const urlParams = new URLSearchParams(window.location.search);

        const which_folder = urlParams.get('type');
        const which_file = urlParams.get('file');

        if ( which_folder && which_file )
        {

            main_Element.innerHTML += `<h2 class="file-title"> main-source.cpp </h2>`;
            main_Element.innerHTML += iframe_Element;
            const iframe = document.getElementById('main-iframe');
            iframe.src = `ALL/${which_folder}/${which_file}/main-source.txt `;

        }
        
    });

