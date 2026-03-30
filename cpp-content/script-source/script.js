

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
            const countDX = dados.directx;
            const countWin = dados.windows;
            const totalFolders = countDX + countWin;

            personal_project_area.innerHTML = "";

            for ( let i = 0 ; i < totalFolders ; i++)
            {
                personal_project_area.innerHTML += "<p>Project Link Placeholder</p>";
            }

        } catch (error) {
            console.error("Erro ao carregar o arquivo local:", error);
        }
    }

    console.log("the javascript is working");