

// GLOBAL VARIABLES

    const personal_project_area = document.getElementById('personal-projects-area');






// CODES

    async function loadProjects()
    {
        try
        {

            // Se estiverem na mesma pasta, use apenas o nome do arquivo
            const response = await fetch('projects.json'); 
        
            if (!response.ok) throw new Error("Não achei o JSON!");

            const dados = await response.json();

            // Aqui você pega os valores que definiu dentro do seu JSON
            let countDX = dados.directx;
            let countWin = dados.windows;
            let totalFolders = countDX + countWin;

            for ( let i = 0 ; i < 5 ; i++ )
            {
                personal_project_area.innerHTML += "<p>Project Link Placeholder</p>";
            }

        } catch (error) {
            console.error("Erro ao carregar o arquivo local:", error);
        }
    }

    loadProjects();

    personal_project_area.innerHTML += "<p>The javascript is working</p>";