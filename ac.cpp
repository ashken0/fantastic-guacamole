#include "ac.h"
#include "threadpool/threadpool.h"
#include <sstream>
threadpool::ThreadPool pool;
//
int ac_calibration::trace(const string &root,const string &rt)
{
	if (root=="-1")
	{
		return 0;
	}

	vector<string> &rfroms = allnode.at(root).from;
	vector<future<int>> vecfut(rfroms.size() - 1);
	for (std::size_t i = 1; i < rfroms.size(); i++)
	{
		// Run a function returning a result
		future<int> f = pool.run(std::bind(&ac_calibration::trace, this, rfroms.at(i),root));
		// ... do some other things in parallel to the thread pool.
		vecfut.at(i - 1)._Swap(f);
		// Wait for the function to finish and get the result
	}
	trace(*rfroms.begin(),root);
	for (vector<future<int>>::iterator it = vecfut.begin(); it != vecfut.end(); it++)
	{
		it->get();
	}
	//������������ ����ӵ�������
	//չʾһ��
	ac_msg tmsg;
	tmsg.tid = std::this_thread::get_id();
	tmsg.cd = root;
	tmsg.rt = rt;
	resnotify.push(tmsg);
	return 1;
}

void ac_calibration::wait()
{
	pool.wait();
}

void ac_calibration::stringsplit(const string & r, vector<string>& v)
{
	stringstream ss(r);
	string s;
	while (getline(ss,s,','))
	{
		if (!s.empty())
		{
			v.push_back(s);
		}
	}
}

class XinanjiangModel
{
private:
	// FORCING
	double *m_pP;   // ��ˮ����
	long m_nSteps;  // ģ��Ҫ���еĲ���(һ��m_nSteps��)
	long steps;
	// OUTPUT
	double *m_pR;   // ������ÿһ�����Ĳ�����(�������) 
	double *m_pRs;  // ÿһ�����ĵر�����(����) 
	double *m_pRi;  // ÿһ����������������ף�
	double *m_pRg;  // ÿһ�����ĵ��¾�����(����) 
	double *m_pE;   // ÿһ����������(����)
	double *m_pQrs;  // ������ڵر�����
	double *m_pQri;   // ���������������������
	double *m_pQrg;  // ������ڵ��¾�����
	double *m_pQ;   // ������ڵ�������
	double m_U;     // for 24h. U=A(km^2)/3.6/delta_t
					// SOIL
	double *m_pW;     // ����������ʪ��
	double *m_pWu;	  // �������ϲ�����ʪ��
	double *m_pWl;	  // �������²������ʶ�
	double *m_pWd;    // �������������ʪ��
	double m_Wum;	 // �������ϲ�������ˮ����
	double m_Wlm;   // �������²�������ˮ����
	double m_Wdm;   // ���������������ˮ������WDM=WM-WUM-WLM 
	double m_Wu, m_Wd, m_Wl;//����
					// EVAPORATION
	double *m_pEu;  // �ϲ����������������ף�
	double *m_pEl;  // �²����������������ף�
	double *m_pEd;  // ������������������ף�
					//runoff
	double *RF;
	// PARAMETER
	double m_Kc;      // ������ɢ��������ʵ����ɢ��ֵ�ı�
	double m_IM;     // ��͸ˮ���ռȫ�������֮��
	double m_B;      // ��ˮ�������ߵķ��Σ�С���򣨼�ƽ�����B0.1����
					 // �е������ƽ���������ڣ�.2~0.3���ϴ����.3~0.4   
	double m_WM;     // ����ƽ����ˮ���������ף�(WM=WUM+WLM+WDM)
	double m_C;      // �����������������ϵ��������ʪ�������0.15-0.2��
					 //������ʪ�������.09-0.12
	double m_SM;    //����ˮ��ˮ����
	double m_EX;    //����ˮ��ˮ����������ֲ�����ָ��
	double m_KG;    //����ˮ�ճ���ϵ��
	double m_KS;    //����ˮ�ճ���ϵ��
	double m_KI;    //�������ճ���ϵ��
	double m_CG;    //����ˮ����ϵ��
	double m_CI;    //����������ϵ��
	double *m_UH;    // ��Ԫ�����ϵ��澶���ĵ�λ��
	double m_WMM;     // �����������ˮ����
	double m_Area;    // �������
	double m_Em;     //����������
	int  m_DeltaT;   // ÿһ������Сʱ��
	int  m_PD;       // �������ݣ������ж��Ƿ�ʱ�кӵ���������
	double m_KKS ,m_KKR,m_KKG;
	double m_S0, m_Fr0;
	double m_Eu, m_El, m_Ed;
	double m_R;
public:
	XinanjiangModel(void);
	~XinanjiangModel(void);
	// ��ʼ��ģ��
	void InitModel(long nSteps, double Area, int DeltaT, int PD, char *ForcingFile);
	// ����ģ�Ͳ���
	void SetParameters(double *Params);
	// �����°���ģ��
	void RunModel(void);
	// ����ģ�������ļ�
	void SaveResults(char *FileName);
	// ��¼�������ݣ�������ͼ����
	void Runoff(char *runoff);
private:
	// ���л������㣬���������ת��Ϊ������ڵ�����
	void Routing(void);

	void evaporation();
};


XinanjiangModel::XinanjiangModel(void)
{
	this->m_pP = NULL;
	this->m_pEm = NULL;
	this->m_pE = NULL;
	this->m_pEd = NULL;
	this->m_pEl = NULL;
	this->m_pEu = NULL;
	this->m_pW = NULL;
	this->m_pWd = NULL;
	this->m_pWl = NULL;
	this->m_pWu = NULL;
	this->m_pR = NULL;
	this->m_pRg = NULL;
	this->m_pRi = NULL;
	this->m_pRs = NULL;
	this->m_pQ = NULL;
	this->m_pQrg = NULL;
	this->m_pQri = NULL;
	this->m_pQrs = NULL;
}
XinanjiangModel::~XinanjiangModel(void)
{
	delete[] this->m_pP;
	delete[] this->m_pEm;
	delete[] this->m_pE;
	delete[] this->m_pEd;
	delete[] this->m_pEl;
	delete[] this->m_pEu;
	delete[] this->m_pW;
	delete[] this->m_pWd;
	delete[] this->m_pWl;
	delete[] this->m_pWu;
	delete[] this->m_pR;
	delete[] this->m_pRg;
	delete[] this->m_pRi;
	delete[] this->m_pRs;
	delete[] this->m_pQ;
	delete[] this->m_pQrg;
	delete[] this->m_pQrs;
	delete[] this->m_pQri;
}
// ��ʼ��ģ��
void XinanjiangModel::InitModel(long nSteps, double Area, int DeltaT, int PD, char  * ForcingFile)
{
	FILE  * fp;
	int i;
	this->m_nSteps = nSteps;
	this->steps = this->m_nSteps + 18;
	// ��������
	this->m_pP = new double[this->steps];
	this->m_pEm = new double[this->steps];
	// ģ���������ɢ����
	this->m_pE = new double[this->steps];
	this->m_pEd = new double[this->steps];
	this->m_pEl = new double[this->steps];
	this->m_pEu = new double[this->steps];
	// ģ���������������������Ĳ���
	this->m_pQrg = new double[this->steps];
	this->m_pQrs = new double[this->steps];
	this->m_pQri = new double[this->steps];
	this->m_pQ = new double[this->steps];
	// ģ�������������
	this->m_pR = new double[this->steps];
	this->m_pRg = new double[this->steps];
	this->m_pRi = new double[this->steps];
	this->m_pRs = new double[this->steps];
	// ģ��״̬��������ʪ��
	this->m_pW = new double[this->steps];
	this->m_pWd = new double[this->steps];
	this->m_pWl = new double[this->steps];
	this->m_pWu = new double[this->steps];
	//runoffֵ
	this->RF = new double[this->steps];
	for (i = 0; i<this->steps; i++)
	{
		// ��������
		this->m_pP[i] = 0.00;
		this->m_pEm[i] = 0.00;
		// ģ���������ɢ����
		this->m_pE[i] = 0.00;
		this->m_pEd[i] = 0.00;
		this->m_pEl[i] = 0.00;
		this->m_pEu[i] = 0.00;
		// ģ���������������������Ĳ���
		this->m_pQrg[i] = 0.00;
		this->m_pQrs[i] = 0.00;
		this->m_pQri[i] = 0.00;
		this->m_pQ[i] = 0.00;
		// ģ�������������
		this->m_pR[i] = 0.00;
		this->m_pRg[i] = 0.00;
		this->m_pRi[i] = 0.00;
		this->m_pRs[i] = 0.00;
		// ģ��״̬��������ʪ��
		this->m_pW[i] = 0.00;
		this->m_pWd[i] = 0.00;
		this->m_pWl[i] = 0.00;
		this->m_pWu[i] = 0.00;
	}
	this->m_Area = Area;
	this->m_DeltaT = DeltaT;
	this->m_PD = PD;
	this->m_U = this->m_Area / (3.6 * this->m_DeltaT);
	// Forcing�ļ���ʽ����һ�У���ˮ����λ���ף��ո�ڶ���ˮ�����������ף�
	if ((fp = fopen(ForcingFile, "r")) == NULL)
	{
		printf("Can not open forcing file!\n"); return;
	}
	for (i = 0; i<this->m_nSteps; i++)
	{
		fscanf(fp, "%lf%lf", &(this->m_pP[i]), &(this->m_pEm[i]));
	}
	fclose(fp);
}
// ����ģ�Ͳ���
void XinanjiangModel::SetParameters(double*  Params)
{
	this->m_Kc = Params[0];   // (1) ������ɢ��������ʵ��ˮ������֮��
	this->m_IM = Params[1];     // (2) ����͸ˮ���ռȫ�������֮��
	this->m_B = Params[2];     // (3) ��ˮ�������ߵķ���
	this->m_Wum = Params[3];     // (4) �ϲ���ˮ����
	this->m_Wlm = Params[4];     // (5) �²���ˮ����
	this->m_Wdm = Params[5];     // (6) �����ˮ����
	this->m_C = Params[6];     // (7) �����ɢ��ϵ��
	this->m_SM = Params[7];     // (8)����ˮ��ˮ����
	this->m_EX = Params[8];   // (9)����ˮ��ˮ����������ֲ�����ָ��
	this->m_KG = Params[9];     // (10)����ˮ�ճ���ϵ��
	this->m_KI = Params[10];    // (11)�������ճ���ϵ��
	this->m_CG = Params[11];    // (12)����ˮ����ϵ��
	this->m_CI = Params[12];    // (13)����������ϵ��
	this->m_WM = this->m_Wum + this->m_Wlm + this->m_Wdm;
	this->m_WMM = this->m_WM * (1.0 + this->m_B) / (1.0 - this->m_IM);
}
// �����°���ģ��
void XinanjiangModel::RunModel(void)
{
	long i;
	// ģ�͵�״̬����
	double PE;  // > 0 ʱΪ������;< 0 Ϊ������������mm��
	double Ep;  //m_Kc  *  m_pEm[i]
	double P;
	double R;   // ������ȣ������ر������������͵��¾�����mm��
	double RB;  // ��͸ˮ���ϲ����ľ�����ȣ�mm��
	double RG;  // ���¾�����ȣ�mm��
	double RI;  // ��������ȣ�mm��
	double RS;  // �ر����mm��
	double A;   //����ʪ��ΪWʱ������ˮ������ɵľ�����ȣ�mm��
	double E = 0.0;   // ��ɢ��(mm)
	double EU = 0.0;   // �ϲ�������ɢ������mm��
	double EL = 0.0;   // �²�������ɢ������mm��
	double ED = 0.0;   // ���������ɢ������mm��
	double S;
	double FRo;
	double FR;
	double MS;
	double AU;
	double WU = 5.0;   // �������ϲ�����ʪ��
	double WL = 55.0;  // �������²������ʶ�
	double WD = 40.0;  // �������������ʪ��
	double W = 100.0;
	double So = 5.0;
	MS = m_SM * (1 + m_EX);
	FRo = 1 - pow((1 - So / MS), m_EX);
	for (i = 0; i<this->m_nSteps; i++)
	{
		// ��������������ɢ�����㡪����������������������//
		RB = m_pP[i] * m_IM;     // RB�ǽ��ڲ�͸ˮ��Ľ�����
		P = m_pP[i] * (1 - m_IM);
	/*	Ep = m_Kc * m_pEm[i];
		if ((WU + P) >= Ep)
		{
			EU = Ep; EL = 0; ED = 0;
		}
		else if ((WU + P)<Ep)
		{
			EU = WU + P;
			if (WL >= (m_C * m_Wlm))
			{
				EL = (Ep - EU) * WL / m_Wlm;  ED = 0;
			}
			else if ((m_C * (Ep - EU)) <= WL&&WL<(m_C * m_Wlm))
			{
				EL = m_C * (Ep - EU);  ED = 0;
			}
			else if (WL<m_C * (Ep - EU))
			{
				EL = WL;  ED = m_C * (Ep - EU) - EL;
			}
		}*/
		E = EU + EL + ED;
		PE = P - E;

		W = WU + WL + WD;
		////��ˮԴ���ֻ�������
		if (PE>0)
		{
			FR = (R - RB) / PE;
			AU = MS * (1 - pow((1 - So * FRo / FR / m_SM), 1 / (1 + m_EX)));
			if (PE + AU<MS)
				RS = FR * (PE + So * FRo / FR - m_SM + m_SM * pow((1 - (PE
					+ AU) / MS), m_EX + 1));
			else if (PE + AU >= MS)
				RS = FR * (PE + So * Fro / FR - m_SM);
			S = So * Fro / FR + (R �C RS) / FR;
			RI = m_KI * S * FR;
			RG = m_KG * S * FR;
			RS += RB;
			R = RS + RI + RG;
			So = S * (1 - m_KI - m_KG);
			FRo = FR;
		}
		else
		{
			S = So;
			FR = 1 - pow((1 �C S / MS), m_EX);
			RI = 0.00;
			RG = 0.00;
			So = S * (1 - m_KI - m_KG);
			RS = RB;
			R = RS + RI + RG;
			FRo = FR;
		}
		////��ˮԴ���ּ������
		/* ���²�����״̬���������������ϡ��º���������������ı���*/
		/* 1 */	this->m_pE[i] = E;     // ��ǰ������������ģ����Ҫ�����
		/* 2 */	this->m_pEu[i] = EU;   // ��ǰ�����ϲ���������
		/* 3 */	this->m_pEl[i] = EL;   // ��ǰ�����²���������
		/* 4 */	this->m_pEd[i] = ED;   // ��ǰ���������������
		/* 5 */	this->m_pW[i] = W;	   // ��ǰ��������ƽ��������ˮ��
		/* 6 */	this->m_pWu[i] = WU;   // ��ǰ���������ϲ�������ˮ��
		/* 7 */	this->m_pWl[i] = WL;   // ��ǰ���������²�������ˮ��
		/* 8 */	this->m_pWd[i] = WD;   // ��ǰ�����������������ˮ��
		/* 9 */	this->m_pRg[i] = RG;   // ��ǰ����������¾������
		/* 10 */ this->m_pRi[i] = RI;   // ��ǰ�����������������
		/* 11 */	this->m_pRs[i] = RS;   // ��ǰ��������ر����������
		/* 12 */ this->m_pR[i] = R;     // ��ǰ�������ܲ����������
	}
	this->Routing();
}
// ���л������㣬���������ת��Ϊ������ڵ�����
void XinanjiangModel::Routing(void)
{
	/////////////    ���澶���������㣺��λ�߷�       ///////////////////////
	int i, j;
	double B[10000] = { 0.00 };
	if (this->m_PD == 1)
	{
		double UH[] = { 3.71,12.99,38.96,94.63,131.74,154.00,166.99,176.27,178.12,
			172.55,146.58, 90.91,53.80, 31.54,18.55, 9.27, 3.71,0.00 };
		for (i = 0; i<this->m_nSteps; i++)
		{
			for (j = 0; j<18; j++)
			{
				B[i + j] += this->m_pRs[i] * UH[j] / 10.0;
			}
		}
	}
	else
	{
		double UH[] = { 7.18,23.38,63.20,143.10,221.75,365.18,447.40,491.29,
			506.93,504.82,468.46,388.56,309.91,166.49,84.26,40.37,17.56,3.46 };
		for (i = 0; i<this->m_nSteps; i++)
		{
			for (j = 0; j<18; j++)
			{
				B[i + j] += this->m_pRs[i] * UH[j] / 10.0;
			}
		}
	}
	for (i = 0; i<this->steps; i++)
		this->m_pQrs[i] = B[i];
	///// ��������������:����ˮ�� 
	for (i = 1; i<this->steps; i++) {
		this->m_pQri[i] = this->m_CI * this->m_pQri[i - 1]
			+ (1.0 - this->m_CI) * this->m_pRi[i] * this->m_U;
	}
	///// ���¾����������㣺����ˮ�� 
	for (i = 1; i<this->steps; i++) {
		this->m_pQrg[i] = this->m_pQrg[i - 1] * this->m_CG
			+ this->m_pRg[i] * (1.0 - this->m_CG) * this->m_U;
	}
	//////��Ԫ�������������
	for (i = 0; i<this->steps; i++)
	{
		this->m_pQ[i] = this->m_pQrs[i] + this->m_pQri[i] + this->m_pQrg[i];
	}
}
//
void XinanjiangModel::evaporation(double &prun)
{
	/* ����������������ɢ������������������������������� */
	double Ep = m_Kc * m_Em;//��ɢ������
	if ((m_Wu + prun) >= Ep)
	{
		m_Eu = Ep; m_El = 0; m_Ed = 0;
	}
	else
	{
		m_Eu = m_Wu + prun;
		if (m_Wl >= (m_C * m_Wlm))
		{
			m_El = (Ep - m_Eu) * m_Wl / m_Wlm;  m_Ed = 0;
		}
		else if ((m_C * (Ep - m_Eu)) <= m_Wl)
		{
			m_El = m_C * (Ep - m_Eu);  m_Ed = 0;
		}
		else
		{
			m_El = m_Wl;  m_Ed = m_C * (Ep - m_Eu) - m_El;
		}
	}
	double PE = prun - m_El - m_Eu - m_Ed;
	//��������������������������㡪����������������������//
	if (PE <= 0)
	{
		m_R = 0;
	}
	else
	{
		double W = m_Wu + m_Wl + m_Wd;
		double A = m_WMM * (1 - pow((1.0 - W / m_WM), 1.0 / (1 + m_B)));
		// ����ʪ�����㾻����+��ˮ������ʣ������<���������ˮ����
		if ((A + PE) < m_WMM)
		{
			// �����ڵĲ�����ȼ���
			m_R = PE             /*  ��ˮ�������ʣ����*/
				+ W          /* ������������ʼ��ˮ����*/
				+ m_WM * pow((1 - (PE + A) / m_WMM), (1 + m_B))
				- m_WM; /* ��ȥ����ƽ����ˮ������m_WM:������  */
		}
		// ����ʪ�����㾻����+��ˮ������ʣ������<���������ˮ����
		else
		{
			// �����ڵĲ�����ȼ���
			m_R = PE             /*  ��ˮ�������ʣ����					              +  W   /*  ����������ʪ��*/
				- m_WM  /*  ��ȥ����ƽ����ˮ����  */
				+ W;
		}
	}
	//������ˮ���ļ���: WU, WL, WD
	if (prun - m_Eu - m_R > 0.0) {
		if (m_Wu + prun - m_Eu - m_R <= m_Wum) {
			m_Wu = m_Wu + prun - m_Eu - m_R;
			m_Wl = m_Wl - m_El;
			m_Wd = m_Wd - m_Ed;
		}
		else {
			m_Wu = m_Wum;
			if ((m_Wl - m_El) + (m_Wu + prun - m_Eu - m_R - m_Wum) <= m_Wlm) {
				m_Wl = (m_Wl - m_El) + (m_Wu + prun - m_Eu - m_R - m_Wum);
				m_Wd = m_Wd - m_Ed;
			}
			else {
				m_Wl = m_Wlm;
				if ((m_Wd - m_Ed) + (m_Wl - m_El) + (m_Wu + prun - m_Eu - m_R - m_Wum) - m_Wlm <= m_Wdm) {
					m_Wd = (m_Wd - m_Ed) + (m_Wl - m_El) + (m_Wu + prun - m_Eu - m_R - m_Wum) - m_Wlm;
				}
				else {
					m_Wd = m_Wdm;
				}
			}
		}
	}
	else {
		if (m_Wu > fabs(prun - m_Eu - m_R)) {
			m_Wu = m_Wu - fabs(prun - m_Eu - m_R);
			m_Wl = m_Wl-m_El;
			m_Wd = m_Wd - m_Ed;
		}
		else {
			m_Wu = 0;
			m_Wl = m_Wu - fabs(prun - m_Eu - m_R) + m_Wl - m_El;
			m_Wd = m_Wd - m_Ed;
			if (m_Wl < 0) {
				m_Wl = 0;
				m_Wd = m_Wu - fabs(prun - m_Eu - m_R) + (m_Wl - m_El) + (m_Wd - m_Ed);
				if (m_Wd < 0) {
					m_Wd = 0;
				}
			}
		}
	}
}

int _tmain(int argc, _TCHAR *  argv[])
{
	long  nSteps = 942;
	int  DeltaT = 24;
	double Area1 = 1603;
	XinanjiangModel Model1;
	Model1.InitModel(nSteps, Area1, DeltaT, 1, "LFForcingfile.txt");
	//ģ�Ͳ���/*Kc,IM,B,m_Wum,Wlm,Wdm,C,SM,EX,KG,KI,CG,CI 
	double Params1[] = { 0.50,0.01,0.30,10,60,40,0.18,32,1.2,0.075,0.072,0.94,0.7 };
	Model1.SetParameters(Params1);
	Model1.RunModel();
	Model1.SaveResults("����վ��ģ�ͼ�����.txt");
	Model1.Runoff("LF_Q.txt");
	Model1.~XinanjiangModel();
	double  Area2 = 2991;
	XinanjiangModel Model2;
	Model2.InitModel(nSteps, Area2, DeltaT, 0, "YCForcingfile.txt");
	//ģ�Ͳ���/*Kc,IM,B,m_Wum,Wlm,Wdm,C,SM,EX,KG,KI,CG,CI  
	double Params2[] = { 0.75,0.01,0.32,10,60,40,0.18,27,1.2,0.065,0.067,0.96,0.8 };
	Model2.SetParameters(Params2);
	Model2.RunModel();
	Model2.SaveResults("file.txt");
	Model2.Runoff("YC_Q.txt");
	Model2.~XinanjiangModel();
	FILE *fp1, *fp2;
	double Q1[1000], Q2[1000], Q[1000] = { 0.00 };
	ofstream outfile;
	outfile.open("Q.txt");
	if ((fp1 = fopen("LF_Q.txt", "r")) == NULL)
	{
		printf("Can not open the file!\n"); return 0;
	}
	if ((fp2 = fopen("YC_Q.txt", "r")) == NULL)
	{
		printf("Can not open the file!\n"); return 0;
	}
	if (outfile.is_open())
	{
		for (int i = 0; i<960; i++)
		{
			fscanf(fp1, "%lf", &Q1[i]);
			fscanf(fp2, "%lf", &Q2[i]);
			Q[i] = Q1[i] + Q2[i];
			outfile << setprecision(3) << setiosflags(ios::fixed) << Q[i] << endl;
		}
		fclose(fp1);
		fclose(fp2);
	}
	outfile.close();
	return 0;
}
